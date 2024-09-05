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

#include "Settings_Def_Stubs.hpp"

#include <jansson.h>

#include <string>

namespace OuroborosModules {
    struct OuroborosSettings {
      public:
        json_t* saveToJson ();
        void readFromJson (json_t* settingsJ);

      private:
        void saveInternal (json_t* rootJ);
        void readInternal (json_t* rootJ);

      public:
#define DEFINE_NONSETTING(varType, varName, defaultValue)       varType     varName = defaultValue;
#define DEFINE_BOOL(varName, jsonName, defaultValue)            bool        varName = defaultValue;
#define DEFINE_INT(varType, varName, jsonName, defaultValue)    varType     varName = defaultValue;
#define DEFINE_ENUM(varType, varName, jsonName, defaultValue)   varType     varName = defaultValue;
#define DEFINE_FLOAT(varType, varName, jsonName, defaultValue)  varType     varName = defaultValue;
#define DEFINE_STD_STRING(varName, jsonName, defaultValue)      std::string varName = defaultValue;
#define DEFINE_CHAR_STRING(varName, jsonName, defaultValue)     char*       varName = defaultValue;
#define DEFINE_STRUCT(varType, varName, jsonName, defaultValue) varType     varName = defaultValue;

#include "Settings_Def.x"

#undef DEFINE_NONSETTING
#undef DEFINE_BOOL
#undef DEFINE_INT
#undef DEFINE_ENUM
#undef DEFINE_FLOAT
#undef DEFINE_STD_STRING
#undef DEFINE_CHAR_STRING
#undef DEFINE_STRUCT
    };

    void initSettings ();

    extern OuroborosSettings pluginSettings;
}

using OuroborosModules::pluginSettings;