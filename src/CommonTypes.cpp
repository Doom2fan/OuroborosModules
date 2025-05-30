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

#include "CommonTypes.hpp"

#include "JsonUtils.hpp"
#include "PluginDef.hpp"

namespace OuroborosModules {
    /*
     * RGBColor
     */
    json_t* RGBColor::dataToJson () const {
        auto rootJ = json_object ();

        json_object_set_new_float (rootJ, "R", r);
        json_object_set_new_float (rootJ, "G", g);
        json_object_set_new_float (rootJ, "B", b);
        json_object_set_new_float (rootJ, "A", a);

        return rootJ;
    }

    bool RGBColor::dataFromJson (json_t* rootJ) {
        if (!json_is_object (rootJ))
            return false;

        json_object_try_get_float (rootJ, "R", r);
        json_object_try_get_float (rootJ, "G", g);
        json_object_try_get_float (rootJ, "B", b);
        json_object_try_get_float (rootJ, "A", a);

        return true;
    }

    /*
     * SoundSettings
     */
    json_t* SoundSettings::dataToJson () const {
        auto rootJ = json_object ();

        json_object_set_new_string (rootJ, "Path", path);
        json_object_set_new_bool (rootJ, "Enabled", enabled);
        json_object_set_new_float (rootJ, "Volume", volume);

        return rootJ;
    }

    bool SoundSettings::dataFromJson (json_t* rootJ) {
        if (!json_is_object (rootJ))
            return false;

        json_object_try_get_string (rootJ, "Path", path);
        json_object_try_get_bool (rootJ, "Enabled", enabled);
        json_object_try_get_float (rootJ, "Volume", volume);

        return true;
    }
}