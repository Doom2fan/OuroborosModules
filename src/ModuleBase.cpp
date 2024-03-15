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

#include "ModuleBase.hpp"
#include "JsonUtils.hpp"

namespace OuroborosModules {
    json_t* ModuleBase::dataToJson () {
        auto rootJ = json_object ();

        // Theming
        json_object_set_new_enum (rootJ, "theme_Override", theme_Override);
        json_object_set_new_enum (rootJ, "theme_Emblem", theme_Emblem);

        return rootJ;
    }

    void ModuleBase::dataFromJson (json_t* rootJ) {
        // Theming
        json_object_try_get_enum (rootJ, "theme_Override", theme_Override);
        json_object_try_get_enum (rootJ, "theme_Emblem", theme_Emblem);
    }
}