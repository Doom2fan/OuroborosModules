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

#include "PluginDef.hpp"

void json_object_try_get_bool (json_t* rootJ, const char* name, bool& value);
void json_object_set_new_bool (json_t* rootJ, const char* name, bool value);

template<typename T>
void json_object_try_get_int (json_t* rootJ, const char* name, T& value) {
    static_assert (std::is_integral<T>::value, "T must be an integral type");

    auto nodeJ = json_object_get (rootJ, name);
    if (!json_is_integer (nodeJ))
        return;

    value = json_integer_value (nodeJ);
}
template<typename T>
void json_object_set_new_int (json_t* rootJ, const char* name, T value) {
    static_assert (std::is_integral<T>::value, "T must be an integral type");
    json_object_set_new (rootJ, name, json_integer (value));
}

template<typename T>
void json_object_try_get_enum (json_t* rootJ, const char* name, T& value) {
    static_assert (std::is_enum<T>::value && std::is_integral<typename std::underlying_type<T>::type>::value, "T must be an integral type");

    auto nodeJ = json_object_get (rootJ, name);
    if (!json_is_integer (nodeJ))
        return;

    value = (T) json_integer_value (nodeJ);
}
template<typename T>
void json_object_set_new_enum (json_t* rootJ, const char* name, T value) {
    static_assert (std::is_enum<T>::value && std::is_integral<typename std::underlying_type<T>::type>::value, "T must be an integral type");
    json_object_set_new (rootJ, name, json_integer ((typename std::underlying_type<T>::type) value));
}

template<typename T>
void json_object_try_get_float (json_t* rootJ, const char* name, T& value) {
    static_assert (std::is_floating_point<T>::value, "T must be a floating point type");

    auto nodeJ = json_object_get (rootJ, name);
    if (!json_is_number (nodeJ))
        return;

    value = json_number_value (nodeJ);
}
template<typename T>
void json_object_set_new_float (json_t* rootJ, const char* name, T value) {
    static_assert (std::is_floating_point<T>::value, "T must be a floating point type");
    json_object_set_new (rootJ, name, json_real (value));
}

void json_object_try_get_string (json_t* rootJ, const char* name, std::string& value);
void json_object_set_new_string (json_t* rootJ, const char* name, std::string value);

void json_object_try_get_string (json_t* rootJ, const char* name, const char*& value);
void json_object_set_new_string (json_t* rootJ, const char* name, const char* value);