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
        *freqSrc = math::clamp (std::roundl (value), minFreq, maxFreq);

        if (setAction != nullptr)
            setAction (value);
    }

    float getValue () override { return *freqSrc; }
    float getMinValue () override { return minFreq; }
    float getMaxValue () override { return maxFreq; }
    float getDefaultValue () override { return 0.0f; }
    float getDisplayValue () override { return getValue (); }

    std::string getDisplayValueString () override {
        float valTime = getDisplayValue ();
        return string::f ("%.0f", valTime);
    }

    void setDisplayValue (float displayValue) override { setValue (displayValue); }
    std::string getLabel () override { return labelText; }
    std::string getUnit () override { return " Hz"; }
};

struct FloatQuantity : rack::Quantity {
  private:
    std::string format;
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

        this->format = string::f ("%%.%df", precision);
        this->setAction = setAction;
    }

    void setValue (float value) override {
        *valueSrc = math::clamp (value, getMinValue (), getMaxValue ());

        if (setAction != nullptr)
            setAction (value);
    }

    float getValue () override { return *valueSrc; }
    float getMinValue () override { return minValue; }
    float getMaxValue () override { return maxValue; }
    float getDefaultValue () override { return 0.0f; }
    float getDisplayValue () override { return getValue (); }

    std::string getDisplayValueString () override {
        float valTime = getDisplayValue ();
        return string::f (format.c_str (), valTime);
    }

    void setDisplayValue (float displayValue) override { setValue (displayValue); }
    std::string getLabel () override { return labelText; }
    std::string getUnit () override { return ""; }
};

char* selectSoundFile ();
ThemeKind getCurrentTheme ();