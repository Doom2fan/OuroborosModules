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

#pragma once

#include "../PluginDef.hpp"

namespace OuroborosModules::UI {
    using DotFontPtr = std::shared_ptr<struct DotFont>;

    using DotFontAlignBase = uint8_t;
    enum class DotFontAlign : DotFontAlignBase {
        Left = 1 << 0,
        HCenter = 1 << 1,
        Right = 1 << 2,
        HAlign = Left | HCenter | Right,

        Top = 1 << 3,
        VCenter = 1 << 4,
        Bottom = 1 << 5,
        VAlign = Top | VCenter | Bottom,
    };

    inline DotFontAlign operator| (const DotFontAlign lhs, const DotFontAlign rhs) {
        return static_cast<DotFontAlign> (static_cast<DotFontAlignBase> (lhs) | static_cast<DotFontAlignBase> (rhs));
    }
    inline DotFontAlign operator& (const DotFontAlign lhs, const DotFontAlign rhs) {
        return static_cast<DotFontAlign> (static_cast<DotFontAlignBase> (lhs) & static_cast<DotFontAlignBase> (rhs));
    }

    static constexpr char32_t DotOnChar  = '#';
    static constexpr char32_t DotOffChar = '.';

    /*
     * Stores and renders a JSON dot-matrix font
     */
    struct DotFont {
      public:
        struct DotChar {
            uint32_t width = 0;
            size_t startIndex = 0;
        };

      private:
        std::unordered_map<char32_t, uint32_t> charMap = { };
        std::vector<DotChar> charList = { };
        std::vector<bool> charData = { };

        uint32_t height;

        inline const DotChar& getChar (char32_t c) const {
            if (auto search = charMap.find (c); search != charMap.end ())
                return charList [search->second];

            return charList [0];
        }

        static constexpr uint32_t calculateIndex (uint32_t width, uint32_t height, uint32_t x, uint32_t y) {
            return y * width + x;
        }

      public:
        static DotFontPtr loadFont (const std::string& path);

        inline uint32_t getHeight () const { return height; }

        /*
         * Renders a string of text, does no line-breaking.
         * Function takes in a center position and the size of a dot
         */
        void renderText (
            std::function<void (rack::Vec, float)> renderFunc,
            const std::string& text,
            rack::Vec textPos, float size,
            float spacing = 1.f, DotFontAlign alignment = DotFontAlign::Top | DotFontAlign::Left,
            float* finalWidth = nullptr
        ) const {
            auto fontScale = size / height;
            auto dotSize = fontScale;
            auto textWidth = 0u;

            // Convert text to UTF-32
            auto textUTF32 = rack::string::UTF8toUTF32 (text);

            // Calculate text width and apply alignment
            for (auto c : textUTF32)
                textWidth += getChar (c).width;
            // Add up all the between-character spacing
            textWidth += (textUTF32.length () - 1.f) * spacing * fontScale;

            switch (alignment & DotFontAlign::HAlign) {
                default:
                case DotFontAlign::Left: break;
                case DotFontAlign::HCenter: textPos.x -= textWidth * fontScale / 2.f; break;
                case DotFontAlign::Right: textPos.x -= textWidth * fontScale; break;
            }
            switch (alignment & DotFontAlign::VAlign) {
                default:
                case DotFontAlign::Top: break;
                case DotFontAlign::VCenter: textPos.y -= height * fontScale / 2.f; break;
                case DotFontAlign::Bottom: textPos.y -= height * fontScale; break;
            }

            // Render text
            auto currentWidth = 0.f;
            for (auto c : textUTF32) {
                auto charInfo = getChar (c);

                for (auto y = 0u; y < height; y++) {
                    auto rowPos = textPos + rack::Vec (0, y * fontScale);

                    for (auto x = 0u; x < charInfo.width; x++) {
                        if (!charData [charInfo.startIndex + calculateIndex (charInfo.width, height, x, y)])
                            continue;

                        auto charPos = rowPos + rack::Vec ((currentWidth + x) * fontScale, 0);
                        renderFunc (charPos, dotSize);
                    }
                }

                currentWidth += charInfo.width + spacing;
            }

            if (finalWidth != nullptr)
                *finalWidth = textWidth * size;
        }
    };
}