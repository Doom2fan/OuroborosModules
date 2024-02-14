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

#include "PluginSettings.hpp"
#include "PluginDef.hpp"
#include "JsonUtils.hpp"
#include <type_traits>

OuroborosSettings pluginSettings;

json_t* OuroborosSettings::saveToJson () {
    auto settingsJ = json_object ();

    saveInternal (settingsJ);

    return settingsJ;
}

void OuroborosSettings::readFromJson (json_t* settingsJ) {
    if (settingsJ == nullptr)
        return;

    readInternal (settingsJ);
}

void OuroborosSettings::saveInternal (json_t* rootJ) {
#define DEFINE_NONSETTING(varType, varName, defaultValue)
#define DEFINE_BOOL(varName, jsonName, defaultValue) json_object_set_new_bool (rootJ, jsonName, varName);
#define DEFINE_INT(varType, varName, jsonName, defaultValue) json_object_set_new_int<varType> (rootJ, jsonName, varName);
#define DEFINE_ENUM(varType, varName, jsonName, defaultValue) json_object_set_new_enum<varType> (rootJ, jsonName, varName);
#define DEFINE_FLOAT(varType, varName, jsonName, defaultValue) json_object_set_new_float<varType> (rootJ, jsonName, varName);
#define DEFINE_STD_STRING(varName, jsonName, defaultValue) json_object_set_new_string (rootJ, jsonName, varName);
#define DEFINE_CHAR_STRING(varName, jsonName, defaultValue) json_object_set_new_string (rootJ, jsonName, varName);

#include "Settings_Def.x"

#undef DEFINE_NONSETTING
#undef DEFINE_BOOL
#undef DEFINE_INT
#undef DEFINE_ENUM
#undef DEFINE_FLOAT
#undef DEFINE_STD_STRING
#undef DEFINE_CHAR_STRING
}

void OuroborosSettings::readInternal (json_t* rootJ) {
#define DEFINE_NONSETTING(varType, varName, defaultValue)
#define DEFINE_BOOL(varName, jsonName, defaultValue) json_object_try_get_bool (rootJ, jsonName, varName);
#define DEFINE_INT(varType, varName, jsonName, defaultValue) json_object_try_get_int<varType> (rootJ, jsonName, varName);
#define DEFINE_ENUM(varType, varName, jsonName, defaultValue) json_object_try_get_enum<varType> (rootJ, jsonName, varName);
#define DEFINE_FLOAT(varType, varName, jsonName, defaultValue) json_object_try_get_float<varType> (rootJ, jsonName, varName);
#define DEFINE_STD_STRING(varName, jsonName, defaultValue) json_object_try_get_string (rootJ, jsonName, varName);
#define DEFINE_CHAR_STRING(varName, jsonName, defaultValue) json_object_try_get_string (rootJ, jsonName, varName);

#include "Settings_Def.x"

#undef DEFINE_NONSETTING
#undef DEFINE_BOOL
#undef DEFINE_INT
#undef DEFINE_ENUM
#undef DEFINE_FLOAT
#undef DEFINE_STD_STRING
#undef DEFINE_CHAR_STRING
}