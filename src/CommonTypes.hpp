/*
 *  OuroborosModules
 *  Copyright (C) 2024 Chronos "phantombeta" Ouroboros
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

#include <cstdint>

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
    };
}