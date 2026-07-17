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

#include "Constants.hpp"

#include <rack.hpp>
#include <jansson.h>

namespace OuroborosModules {
    struct CableGlowSettings {
        // Cable glow settings
        bool glow_Enabled = false;
        float glow_Intensity = .5f;
        bool glow_FollowSignal = true;

        // Cable light settings
        bool lights_Enabled = false;
        float lights_Intensity = .75f;

        bool needHandler () { return glow_Enabled | lights_Enabled; }

        json_t* dataToJson () const;
        bool dataFromJson (json_t* rootJ);
    };
}