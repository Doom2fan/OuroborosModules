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

#include <fmt/format.h>

#include <limits>
#include <cstdint>

namespace OuroborosModules {
namespace Widgets {
    struct SimpleSlider : rack::ui::Slider {
        SimpleSlider (rack::Quantity* quantity) {
            this->quantity = quantity;
        }

        ~SimpleSlider () {
            delete quantity;
        }
    };

    struct UpdateFrequencyQuantity : rack::Quantity {
      private:
        std::string labelText;
        int* freqSrc;
        int minFreq;
        int maxFreq;
        std::function<void (bool)> setAction;

      public:
        UpdateFrequencyQuantity (std::string labelText, int* freqSrc, int minFreq, int maxFreq, std::function<void (float)> setAction = nullptr) {
            this->labelText = labelText;
            this->freqSrc = freqSrc;
            this->minFreq = minFreq;
            this->maxFreq = maxFreq;
            this->setAction = setAction;
        }

        void setValue (float value) override {
            *freqSrc = rack::math::clamp (std::roundl (value), minFreq, maxFreq);

            if (setAction != nullptr)
                setAction (value);
        }

        float getValue () override { return *freqSrc; }
        float getMinValue () override { return minFreq; }
        float getMaxValue () override { return maxFreq; }
        float getDefaultValue () override { return 0.0f; }
        float getDisplayValue () override { return getValue (); }

        std::string getDisplayValueString () override {
            float valFreq = getDisplayValue ();
            return fmt::format (FMT_STRING ("{:.0F}"), valFreq);
        }

        void setDisplayValue (float displayValue) override { setValue (displayValue); }
        std::string getLabel () override { return labelText; }
        std::string getUnit () override { return " Hz"; }
    };

    struct FloatQuantity : rack::Quantity {
      private:
        int precision;
        std::string labelText;
        float* valueSrc;
        float minValue;
        float maxValue;
        std::function<void (bool)> setAction;

      public:
        FloatQuantity (std::string labelText, float* valueSrc, float minValue, float maxValue, int precision, std::function<void (float)> setAction = nullptr) {
            this->labelText = labelText;
            this->valueSrc = valueSrc;
            this->minValue = minValue;
            this->maxValue = maxValue;

            this->precision = precision;
            this->setAction = setAction;
        }

        void setValue (float value) override {
            *valueSrc = rack::math::clamp (value, getMinValue (), getMaxValue ());

            if (setAction != nullptr)
                setAction (value);
        }

        float getValue () override { return *valueSrc; }
        float getMinValue () override { return minValue; }
        float getMaxValue () override { return maxValue; }
        float getDefaultValue () override { return 0.0f; }
        float getDisplayValue () override { return getValue (); }

        std::string getDisplayValueString () override {
            float value = getDisplayValue ();
            return fmt::format (FMT_STRING ("{:.{}f}"), value, precision);
        }

        void setDisplayValue (float displayValue) override { setValue (displayValue); }
        std::string getLabel () override { return labelText; }
        std::string getUnit () override { return ""; }
    };
}
}

namespace OuroborosModules {
namespace Hashing {
    template<typename T>
    T xorshift (const T& n, int i) {
        return n ^ (n >> i);
    }

    std::size_t distribute (const std::size_t& n);

    template<typename T, typename S>
        typename std::enable_if<std::is_unsigned<T>::value, T>::type
    constexpr rotl (const T n, const S i) {
        const T m = (std::numeric_limits<T>::digits - 1);
        const T c = i & m;
        // This is usually recognized by the compiler to mean rotation.
        return (n << c) | (n >> ((T (0) - c) & m));
    }

    // Call this function with the old seed and the new key to be hashed and combined into the new seed value,
    // respectively the final hash.
    template<class T>
    inline std::size_t hashCombine (std::size_t& seed, const T& v) {
        return rotl (seed, std::numeric_limits<size_t>::digits / 3) ^ distribute (std::hash<T> {} (v));
    }

    inline void hashCombine (std::size_t& seed) { }

    template<typename T, typename... Rest>
    inline void hashCombine (std::size_t& seed, const T& v, Rest... rest) {
        seed = hashCombine (seed, v);
        hashCombine (seed, rest...);
    }
}
}

namespace OuroborosModules {
    char* selectSoundFile ();

    template<class TModuleWidget>
    rack::plugin::Model* createModel (std::string slug) {
        return rack::createModel<typename TModuleWidget::_ModuleType, TModuleWidget> (slug);
    }
}

namespace std {
    template<>
    struct hash<NVGcolor> {
        inline std::size_t operator () (const NVGcolor& color) const {
            std::size_t seed = static_cast<size_t> (0);
            OuroborosModules::Hashing::hashCombine (seed, color.r, color.g, color.b, color.a);
            return seed;
        }
    };
}

inline bool operator== (const NVGcolor& lhs, const NVGcolor& rhs) {
    return lhs.r == rhs.r && lhs.g == rhs.g && lhs.b == rhs.b && lhs.a == rhs.a;
}