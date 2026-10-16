/*
 *  OuroborosModules
 *  Copyright (C) 2026 Chronos "phantombeta" Ouroboros
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "DotFont.hpp"

namespace OuroborosModules::UI {
    std::unordered_map<std::string, DotFontPtr> fontCache = { };

    DotFontPtr DotFont::loadFont (const std::string& path) {
        auto canonPath = rack::system::getCanonical (path);

        if (auto search = fontCache.find (canonPath); search != fontCache.end ())
            return search->second;

        auto jsonFile = std::fopen (canonPath.c_str (), "r");
        if (jsonFile == nullptr) {
            LOG_WARN (FMT_STRING ("Dot matrix font at path \"{}\" could not be found"), path);
            return nullptr;
        }
        DEFER ({ std::fclose (jsonFile); });

        json_error_t jsonError;
        auto fontJ = json_loadf (jsonFile, 0, &jsonError);
        if (fontJ == nullptr) {
            LOG_WARN (FMT_STRING ("Error loading dot matrix font at path \"{}\" at {}:{} - {}"),
                path, jsonError.line, jsonError.column, jsonError.text);
            return nullptr;
        }
        DEFER ({ json_decref (fontJ); });

        auto fontData = std::make_shared<DotFont> ();
        fontData->charList.push_back (DotChar ());

        if (json_object_size (fontJ) < 1) {
            LOG_WARN (FMT_STRING ("Error loading dot matrix font at path \"{}\": Font has no characters"), path);
            return nullptr;
        }

        auto& charMap = fontData->charMap;
        auto& charList = fontData->charList;
        auto& charData = fontData->charData;

        std::optional<uint32_t> height = std::nullopt;
        std::vector<bool> charDots;
        const char* charKey;
        json_t* charDataJ;
        json_object_foreach (fontJ, charKey, charDataJ) {
            auto c = rack::string::UTF8toUTF32 (charKey);

            if (c.length () != 1) {
                LOG_WARN (FMT_STRING ("Error loading dot matrix font at path \"{}\": Invalid character \"{}\""),
                    path, charKey);
                return nullptr;
            }

            if (!json_is_array (charDataJ)) {
                LOG_WARN (FMT_STRING ("Error loading dot matrix font at path \"{}\": Character \"{}\" is not array"),
                    path, charKey);
                return nullptr;
            }

            std::optional<uint32_t> width = std::nullopt;

            if (!height)
                height = json_array_size (charDataJ);
            else if (json_array_size (charDataJ) != *height) {
                LOG_WARN (FMT_STRING ("Error loading dot matrix font at path \"{}\": Character \"{}\" has size {}, previous have {}"),
                    path, charKey, json_array_size (charDataJ), *height);
                return nullptr;
            }
            charDots.clear ();

            size_t rowIndex;
            json_t* rowJ;
            json_array_foreach (charDataJ, rowIndex, rowJ) {
                if (!json_is_string (rowJ)) {
                    LOG_WARN (FMT_STRING ("Error loading dot matrix font at path \"{}\": Character \"{}\" row {} is not a string"),
                        path, charKey, rowIndex);
                    return nullptr;
                }

                auto rowData = rack::string::UTF8toUTF32 (json_string_value (rowJ));
                if (!width) {
                    width = rowData.length ();
                    charDots.resize (*width * *height);
                } else if (rowData.length () != *width) {
                    LOG_WARN (FMT_STRING ("Error loading dot matrix font at path \"{}\": Character \"{}\" row {} has mismatched width"),
                        path, charKey, rowIndex);
                    return nullptr;
                }

                auto y = static_cast<uint32_t> (rowIndex);
                for (auto x = 0u; x < *width; x++) {
                    bool dotOn;
                    if (rowData [x] == DotOnChar)
                        dotOn = true;
                    else if (rowData [x] == DotOffChar)
                        dotOn = false;
                    else {
                        LOG_WARN (FMT_STRING ("Error loading dot matrix font at path \"{}\": Character \"{}\" invalid pixel at {}:{}"),
                            path, charKey, x, y);
                        return nullptr;
                    }

                    charDots [calculateIndex (*width, *height, x, y)] = dotOn;
                }
            }

            auto charInfo = DotChar ();
            charInfo.width = *width;
            charInfo.startIndex = charData.size ();
            charData.insert (charData.end (), charDots.begin (), charDots.end ());

            charMap [c [0]] = charList.size ();
            charList.push_back (charInfo);
        }

        if (!height) {
            LOG_WARN (FMT_STRING ("Error loading dot matrix font at path \"{}\": Font has no height?"), path);
            return nullptr;
        }

        // Remap missing lower and uppercase characters
        auto newChars = std::unordered_map<char32_t, uint32_t> ();
        for (auto cKVP : charMap) {
            auto c = cKVP.first;
            auto c8 = rack::string::UTF32toUTF8 (std::u32string (1, c));
            auto lower = rack::string::UTF8toUTF32 (rack::string::lowercase (c8)) [0];
            auto upper = rack::string::UTF8toUTF32 (rack::string::uppercase (c8)) [0];

            // Uncased character
            if (lower == upper && lower == c)
                continue;
            else if (c == upper && !charMap.count (lower) && !newChars.count (lower))
                newChars [lower] = cKVP.second;
            else if (c == lower && !charMap.count (upper) && !newChars.count (upper))
                newChars [upper] = cKVP.second;
        }

        charMap.merge (newChars);

        fontData->height = *height;
        fontCache [canonPath] = fontData;

        return fontData;
    }
}