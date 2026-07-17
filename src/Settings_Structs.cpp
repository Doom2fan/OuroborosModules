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

#include "Settings_Structs.hpp"

#include "PluginDef.hpp"
#include "JsonUtils.hpp"

namespace OuroborosModules {
    /*
     * CableGlowSettings
     */
    json_t* CableGlowSettings::dataToJson () const {
        auto rootJ = json_object ();

        json_object_set_new_bool (rootJ, "Glow::Enabled", glow_Enabled);
        json_object_set_new_float (rootJ, "Glow::Intensity", glow_Intensity);
        json_object_set_new_bool (rootJ, "Glow::FollowSignal", glow_FollowSignal);

        json_object_set_new_bool (rootJ, "Lights::Enabled", lights_Enabled);
        json_object_set_new_float (rootJ, "Lights::Intensity", lights_Intensity);

        return rootJ;
    }

    bool CableGlowSettings::dataFromJson (json_t* rootJ) {
        if (!json_is_object (rootJ))
            return false;

        json_object_try_get_bool (rootJ, "Glow::Enabled", glow_Enabled);
        json_object_try_get_float (rootJ, "Glow::Intensity", glow_Intensity);
        json_object_try_get_bool (rootJ, "Glow::FollowSignal", glow_FollowSignal);

        json_object_try_get_bool (rootJ, "Lights::Enabled", lights_Enabled);
        json_object_try_get_float (rootJ, "Lights::Intensity", lights_Intensity);

        return true;
    }
}