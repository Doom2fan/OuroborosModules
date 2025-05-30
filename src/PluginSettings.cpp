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

#include "PluginSettings.hpp"

#include "JsonUtils.hpp"
#include "PluginDef.hpp"

#include <type_traits>

namespace OuroborosModules {
    OuroborosSettings pluginSettings;

    void tryLoadDefaults () {
        auto defaultsFilePath = rack::asset::user ("OuroborosModules_Default.json");
        if (defaultsFilePath.empty ())
            return;

        FILE* jsonFile = std::fopen (defaultsFilePath.c_str (), "r");
        if (jsonFile == nullptr)
            return;
        DEFER ({ std::fclose (jsonFile); });

        json_error_t jsonError;
        auto defaultsJson = json_loadf (jsonFile, 0, &jsonError);
        if (defaultsJson == nullptr) {
            WARN ("OuroborosModules default settings file error at %d:%d - %s", jsonError.line, jsonError.column, jsonError.text);
            return;
        }

        DEFER ({ json_decref (defaultsJson); });

        pluginSettings.readFromJson (defaultsJson);
    }

    void initSettings () {
        pluginSettings = OuroborosSettings ();
        tryLoadDefaults ();
    }

    json_t* OuroborosSettings::saveToJson () {
        auto settingsJ = json_object ();

        saveInternal (settingsJ);

        return settingsJ;
    }

    void OuroborosSettings::readFromJson (json_t* settingsJ) {
        if (!json_is_object (settingsJ))
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
#define DEFINE_STRUCT(varType, varName, jsonName, defaultValue) json_object_set_new_struct (rootJ, jsonName, varName);

#include "Settings_Def.x"

#undef DEFINE_NONSETTING
#undef DEFINE_BOOL
#undef DEFINE_INT
#undef DEFINE_ENUM
#undef DEFINE_FLOAT
#undef DEFINE_STD_STRING
#undef DEFINE_CHAR_STRING
#undef DEFINE_STRUCT
    }

    void OuroborosSettings::readInternal (json_t* rootJ) {
#define DEFINE_NONSETTING(varType, varName, defaultValue)
#define DEFINE_BOOL(varName, jsonName, defaultValue) json_object_try_get_bool (rootJ, jsonName, varName);
#define DEFINE_INT(varType, varName, jsonName, defaultValue) json_object_try_get_int<varType> (rootJ, jsonName, varName);
#define DEFINE_ENUM(varType, varName, jsonName, defaultValue) json_object_try_get_enum<varType> (rootJ, jsonName, varName);
#define DEFINE_FLOAT(varType, varName, jsonName, defaultValue) json_object_try_get_float<varType> (rootJ, jsonName, varName);
#define DEFINE_STD_STRING(varName, jsonName, defaultValue) json_object_try_get_string (rootJ, jsonName, varName);
#define DEFINE_CHAR_STRING(varName, jsonName, defaultValue) json_object_try_get_string (rootJ, jsonName, varName);
#define DEFINE_STRUCT(varType, varName, jsonName, defaultValue) json_object_try_get_struct (rootJ, jsonName, varName);

#include "Settings_Def.x"

#undef DEFINE_NONSETTING
#undef DEFINE_BOOL
#undef DEFINE_INT
#undef DEFINE_ENUM
#undef DEFINE_FLOAT
#undef DEFINE_STD_STRING
#undef DEFINE_CHAR_STRING
#undef DEFINE_STRUCT
    }
}