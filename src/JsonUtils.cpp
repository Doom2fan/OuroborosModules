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

#include "JsonUtils.hpp"

namespace OuroborosModules {
    void json_object_try_get_bool (json_t* rootJ, const char* name, bool& value) {
        auto nodeJ = json_object_get (rootJ, name);
        if (json_is_true (nodeJ))
            value = true;
        else if (json_is_false (nodeJ))
            value = false;
    }

    void json_object_set_new_bool (json_t* rootJ, const char* name, bool value) {
        json_object_set_new (rootJ, name, json_boolean (value));
    }

    void json_object_try_get_string (json_t* rootJ, const char* name, std::string& value) {
        auto nodeJ = json_object_get (rootJ, name);
        if (!json_is_string (nodeJ))
            return;

        value = json_string_value (nodeJ);
    }

    void json_object_set_new_string (json_t* rootJ, const char* name, std::string value) {
        json_object_set_new (rootJ, name, json_string (value.c_str ()));
    }

    void json_object_try_get_string (json_t* rootJ, const char* name, const char*& value) {
        auto nodeJ = json_object_get (rootJ, name);
        if (!json_is_string (nodeJ))
            return;

        value = json_string_value (nodeJ);
    }

    void json_object_set_new_string (json_t* rootJ, const char* name, const char* value) {
        json_object_set_new (rootJ, name, json_string (value));
    }
}