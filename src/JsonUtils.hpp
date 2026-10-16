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

#include "PluginDef.hpp"
#include "MoreTypeTraits.hpp"

namespace OuroborosModules {
    template<typename T>
    bool json_try_get_auto (json_t* nodeJ, T& value);
    template<typename T>
    json_t* json_create_auto (const T& value);

    /*
     * Array getters
     */
    template<typename T>
    bool json_try_get_array (
        json_t* nodeJ,
        T* value, size_t arraySize,
        std::function<void (size_t)> setDefault = nullptr, bool smallerFails = false
    ) {
        if (!json_is_array (nodeJ))
            return false;

        auto failed = false;
        auto jsonArraySize = json_array_size (nodeJ);
        for (size_t index = 0; index < arraySize; index++) {
            if (index >= jsonArraySize) {
                if (setDefault != nullptr)
                    setDefault (index);

                failed |= smallerFails;
            } else if (!json_try_get_auto (json_array_get (nodeJ, index), value [index])) {
                if (setDefault != nullptr)
                    setDefault (index);

                failed = true;
            }
        }

        return !failed;
    }

    template<typename T, size_t TSize>
    bool json_try_get_array (
        json_t* nodeJ,
        T (&value) [TSize],
        std::function<void (size_t)> setDefault = nullptr, bool smallerFails = false
    ) {
        return json_try_get_array<T> (nodeJ, value, std::size (value), setDefault, smallerFails);
    }

    template<typename T, size_t TSize>
    bool json_try_get_array (
        json_t* nodeJ,
        std::array<T, TSize>& value,
        std::function<void (size_t)> setDefault = nullptr, bool smallerFails = false
    ) {
        return json_try_get_array<T> (nodeJ, value.data (), value.size (), setDefault, smallerFails);
    }

    /*
     * Array creators
     */
    template<typename T>
    json_t* json_create_array (const T* value, size_t arraySize) {
        auto nodeJ = json_array ();

        for (size_t i = 0; i < arraySize; i++) {
            auto elementJ = json_create_auto (value [i]);
            assert (elementJ != nullptr);

            json_array_append (nodeJ, elementJ);
        }

        return nodeJ;
    }

    template<typename T>
    json_t* json_create_array (const T& value) {
        if constexpr (std::is_array_v<T>)
            return json_create_array (value, std::size (value));
        else if constexpr (is_std_array<T>::value)
            return json_create_array (value.data (), value.size ());
    }

    /*
     * Vector getters and setters
     */
    template<typename T>
    bool json_try_get_vector (json_t* nodeJ, std::vector<T>& value) {
        if (!json_is_array (nodeJ))
            return false;

        value.clear ();
        value.reserve (json_array_size (nodeJ));

        auto failed = false;
        size_t index;
        json_t* elementJ;
        json_array_foreach (nodeJ, index, elementJ) {
            T newElement;
            if (!json_try_get_auto (elementJ, newElement)) {
                failed = true;
                continue;
            }

            value.push_back (newElement);
        }

        value.shrink_to_fit ();

        return !failed;
    }

    template<typename T>
    json_t* json_create_vector (const std::vector<T>& value) {
        auto nodeJ = json_array ();

        for (size_t i = 0; i < value.size (); i++) {
            auto elementJ = json_create_auto (value [i]);
            assert (elementJ != nullptr);

            json_array_append (nodeJ, elementJ);
        }

        return nodeJ;
    }

    /*
     * Auto getter
     */
    template<typename T>
    bool json_try_get_auto (json_t* nodeJ, T& value) {
        if constexpr (std::is_same<T, bool>::value) {
            if (!json_is_boolean (nodeJ))
                return false;

            value = json_boolean_value (nodeJ);
            return true;
        } else if constexpr (std::is_integral<T>::value) {
            if (!json_is_integer (nodeJ))
                return false;

            value = json_integer_value (nodeJ);
            return true;
        } else if constexpr (std::is_floating_point<T>::value) {
            if (!json_is_number (nodeJ))
                return false;

            value = json_number_value (nodeJ);
            return true;
        } else if constexpr (std::is_enum<T>::value) {
            static_assert (std::is_integral<typename std::underlying_type<T>::type>::value, "T must be an integral type");
            if (!json_is_integer (nodeJ))
                return false;

            value = (T) json_integer_value (nodeJ);
            return true;
        } else if constexpr (std::is_same<T, std::string>::value || std::is_same<T, const char*>::value || std::is_same<T, char*>::value) {
            if (!json_is_string (nodeJ))
                return false;

            value = json_string_value (nodeJ);
            return true;
        } else if constexpr (is_std_array<T>::value)
            return json_try_get_array (nodeJ, value);
        else if constexpr (is_std_vector<T>::value)
            return json_try_get_vector (nodeJ, value);
        else if constexpr (std::is_class_v<T>)
            return value.dataFromJson (nodeJ);

        return false;
    }

    /*
     * Auto creator
     */
    template<typename T>
    json_t* json_create_auto (const T& value) {
        if constexpr (std::is_same_v<T, bool>)
            return json_boolean (value);
        else if constexpr (std::is_integral_v<T>)
            return json_integer (value);
        else if constexpr (std::is_floating_point_v<T>)
            return json_real (value);
        else if constexpr (std::is_enum_v<T>) {
            static_assert (std::is_integral_v<std::underlying_type_t<T>>, "T must be an integral type");
            return json_integer (static_cast<std::underlying_type_t<T>> (value));
        } else if constexpr (std::is_same<T, std::string>::value)
            return json_string (value.c_str ());
        else if constexpr (std::is_same_v<T, const char*> || std::is_same_v<T, char*>)
            return json_string (value);
        else if constexpr (std::is_array_v<T> || is_std_array<T>::value)
            return json_create_array (value);
        else if constexpr (is_std_vector<T>::value)
            return json_create_vector (value);
        else if constexpr (std::is_class_v<T>)
            return value.dataToJson ();

        return nullptr;
    }

    /*
     * Object auto getters and setters
     */
    template<typename T>
    bool json_object_try_get_auto (json_t* rootJ, const char* name, T& value) {
        auto nodeJ = json_object_get (rootJ, name);
        if (nodeJ == nullptr)
            return false;

        return json_try_get_auto (nodeJ, value);
    }

    template<typename T>
    void json_object_set_new_auto (json_t* rootJ, const char* name, const T& value) {
        auto nodeJ = json_create_auto (value);
        assert (nodeJ != nullptr);

        json_object_set_new (rootJ, name, nodeJ);
    }

    /*
     * Object array getters
     */
    template<typename T>
    bool json_object_try_get_array (
        json_t* rootJ, const char* name,
        T* value, size_t arraySize,
        std::function<void (size_t)> setDefault = nullptr, bool smallerFails = false
    ) {
        auto nodeJ = json_object_get (rootJ, name);
        if (nodeJ == nullptr)
            return false;

        return json_try_get_array (nodeJ, value, arraySize, setDefault, smallerFails);
    }

    template<typename T, size_t TSize>
    bool json_object_try_get_array (
        json_t* rootJ, const char* name,
        T (&value) [TSize],
        std::function<void (size_t)> setDefault = nullptr, bool smallerFails = false
    ) {
        return json_object_try_get_array<T> (rootJ, name, value, std::size (value), setDefault, smallerFails);
    }

    template<typename T, size_t TSize>
    bool json_object_try_get_array (
        json_t* rootJ, const char* name,
        std::array<T, TSize>& value,
        std::function<void (size_t)> setDefault = nullptr, bool smallerFails = false
    ) {
        return json_object_try_get_array<T> (rootJ, name, value.data (), value.size (), setDefault, smallerFails);
    }

    /*
     * Object array setters
     */
    template<typename T>
    void json_object_set_new_array (json_t* rootJ, const char* name, T* value, size_t arraySize) {
        auto nodeJ = json_create_array (value, arraySize);
        assert (nodeJ != nullptr);

        json_object_set_new (rootJ, name, nodeJ);
    }

    template<typename T>
    void json_object_set_new_array (json_t* rootJ, const char* name, T& value) {
        auto nodeJ = json_create_array (value);
        assert (nodeJ != nullptr);

        json_object_set_new (rootJ, name, nodeJ);
    }

    /*
     * Object vector getters and setters
     */
    template<typename T>
    bool json_object_try_get_vector (json_t* rootJ, const char* name, std::vector<T>& value) {
        return json_try_get_vector (rootJ, value, std::size (value));
    }

    template<typename T>
    void json_object_set_new_vector (json_t* rootJ, const char* name, T* value) {
        auto nodeJ = json_create_vector (value);
        assert (nodeJ != nullptr);

        json_object_set_new (rootJ, name, nodeJ);
    }

}