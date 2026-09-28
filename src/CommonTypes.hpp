/*
 *  OuroborosModules
 *  Copyright (C) 2024-2025 Chronos "phantombeta" Ouroboros
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

#include <jansson.h>
#include <nanovg.h>

#include <cassert>
#include <cstdint>
#include <string>

namespace OuroborosModules {
    struct RGBColor {
        union {
            float rgba [4];
            struct { float r, g, b, a; };
        };

        RGBColor () : r (0), g (0), b (0), a (0) { }

        RGBColor (float r, float g, float b) : r (r), g (g), b (b), a (1) { }
        RGBColor (float r, float g, float b, float a) : r (r), g (g), b (b), a (a) { }

        RGBColor (uint8_t r, uint8_t g, uint8_t b) : r (r / 255.f), g (g / 255.f), b (b / 255.f), a (1) { }
        RGBColor (uint8_t r, uint8_t g, uint8_t b, uint8_t a) : r (r / 255.f), g (g / 255.f), b (b / 255.f), a (a / 255.f) { }

        RGBColor (NVGcolor color) : r (color.r), g (color.g), b (color.b), a (color.a) { }

        operator NVGcolor () const {
            NVGcolor color;
            color.r = r;
            color.g = g;
            color.b = b;
            color.a = a;
            return color;
        }

        json_t* dataToJson () const;
        bool dataFromJson (json_t* rootJ);

        bool operator== (const RGBColor& rhs) const;
        bool operator!= (const RGBColor& rhs) { return !(*this == rhs); }
    };

    struct SoundSettings {
        std::string path;
        bool enabled;
        float volume;

        SoundSettings (std::string path, bool enabled, float volume = 1.f)
            : path (path), enabled (enabled), volume (volume) { }

        std::string getPath ();

        json_t* dataToJson () const;
        bool dataFromJson (json_t* rootJ);
    };

    struct DisplayColor {
      private:
        bool isDefault;
        bool isGlobal;
        RGBColor localColor;

        DisplayColor (bool isDefault, bool isGlobal)
            : isDefault (isDefault), isGlobal (isGlobal), localColor (RGBColor (1.f, 1.f, 1.f)) { }

      public:
        static DisplayColor createDefault () { return DisplayColor (true, false); }
        static DisplayColor createGlobal () { return DisplayColor (false, true); }
        static DisplayColor createLocal (RGBColor color) { return DisplayColor (color); }

        DisplayColor () = default;
        DisplayColor (RGBColor color) : isDefault (false), isGlobal (false), localColor (color) { }
        DisplayColor (NVGcolor color) : isDefault (false), isGlobal (false), localColor (color) { }

        bool checkDefault () const { return isDefault; }
        bool checkGlobal () const { return isGlobal; }
        bool checkLocal () const { return !isDefault && !isGlobal; }

        RGBColor getLocal () const { return localColor; }

        RGBColor getColor (DisplayColor* defaultColor, DisplayColor* globalColor) const {
            if (checkDefault ()) {
                assert (defaultColor != nullptr);
                if (defaultColor != nullptr)
                    return defaultColor->getColor (nullptr, globalColor);
            }
            if (checkGlobal ()) {
                assert (globalColor != nullptr);
                if (globalColor != nullptr)
                    return globalColor->getLocal ();
            }

            return getLocal ();
        }

        json_t* dataToJson () const;
        bool dataFromJson (json_t* rootJ);

        bool operator== (const DisplayColor& rhs) const;
        inline bool operator!= (const DisplayColor& rhs) { return !(*this == rhs); }
    };
}