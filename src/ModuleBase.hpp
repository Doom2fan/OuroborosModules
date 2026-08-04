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

#include <sst/rackhelpers/neighbor_connectable.h>

namespace OuroborosModules {
    using SST_NeighborConnectable_V1 = sst::rackhelpers::module_connector::NeighborConnectable_V1;

    struct ModuleBase : rack::engine::Module {
      public:
        ThemeId theme_Override = ThemeId::getUnknown ();
        EmblemId theme_Emblem = EmblemId::getUnknown ();

        json_t* dataToJson () override;
        void dataFromJson (json_t* rootJ) override;

        template <class TParamQuantity = rack::engine::ParamQuantity>
        TParamQuantity* configParamSnap (
            int paramId,
            float minValue, float maxValue, float defaultValue,
            std::string name = "", std::string unit = "",
            float displayBase = 0.f, float displayMultiplier = 1.f, float displayOffset = 0.f
        ) {
            assert (paramId < (int) params.size () && paramId < (int) paramQuantities.size ());

            auto quantity = configParam<TParamQuantity> (
                paramId,
                minValue, maxValue, defaultValue,
                name, unit,
                displayBase, displayMultiplier, displayOffset
            );
            quantity->ParamQuantity::snapEnabled = true;
            quantity->ParamQuantity::smoothEnabled = false;

            return quantity;
        }

        template <class TParamQuantity = rack::engine::ParamQuantity>
        TParamQuantity* configParamDecibels (
            int paramId,
            float minValue, float maxValue, float defaultValue,
            std::string name = ""
        ) {
            assert (paramId < (int) params.size () && paramId < (int) paramQuantities.size ());

            return configParam<TParamQuantity> (
                paramId,
                rack::dsp::dbToAmplitude (minValue), rack::dsp::dbToAmplitude (maxValue), rack::dsp::dbToAmplitude (defaultValue),
                name, " dB",
                -10, 20, 0.f
            );
        }

        template <class TParamQuantity = rack::engine::ParamQuantity>
        TParamQuantity* configParamVOctFromHz (
            int paramId,
            float minValue, float maxValue, float defaultValue,
            std::string name = "", float baseFreq = rack::dsp::FREQ_C4
        ) {
            assert (paramId < (int) params.size () && paramId < (int) paramQuantities.size ());
            auto toVOct = [=] (float freq) { return std::log2 (freq / baseFreq); };

            return configParam<TParamQuantity> (
                paramId,
                toVOct (minValue), toVOct (maxValue), toVOct (defaultValue),
                name, " Hz",
                2, baseFreq, 0.f
            );
        }

        /*
         * Param getters and setters
         */
        float getParam (int idx) { return params [idx].getValue (); }
        void setParam (int idx, float value) { params [idx].setValue (value); }

        /*
         * Common input and output getters and setters
         */
        int getInputChannels (int idx) { return inputs [idx].getChannels (); }
        int getOutputChannels (int idx) { return outputs [idx].getChannels (); }

        bool isInputConnected (int idx) { return getInputChannels (idx) > 0; }
        bool isOutputConnected (int idx) { return getOutputChannels (idx) > 0; }

        bool isInputMonophonic (int idx) { return getInputChannels (idx) == 1; }
        bool isOutputMonophonic (int idx) { return getOutputChannels (idx) == 1; }

        bool isInputPolyphonic (int idx) { return getInputChannels (idx) > 1; }
        bool isOutputPolyphonic (int idx) { return getOutputChannels (idx) > 1; }

        /*
         * Input getters
         */
        float getInput (int idx, uint8_t channel = 0) { return inputs [idx].getVoltage (channel); }
        float getInputPoly (int idx, uint8_t channel) { return inputs [idx].getPolyVoltage (channel); }
        float getInputNormal (int idx, float normalVoltage, uint8_t channel = 0) {
            return inputs [idx].getNormalVoltage (normalVoltage, channel);
        }
        float getInputNormalPoly (int idx, float normalVoltage, uint8_t channel) {
            return inputs [idx].getNormalPolyVoltage (normalVoltage, channel);
        }

        void readInput (int idx, float* v) { inputs [idx].readVoltages (v); }
        float getInputSum (int idx) { return inputs [idx].getVoltageSum (); }
        float getInputRMS (int idx) { return inputs [idx].getVoltageRMS (); }

        template<typename T>
        T getInputSimd (int idx, uint8_t firstChannel) {
            return inputs [idx].getVoltageSimd<T> (firstChannel);
        }

        template<typename T>
        T getInputPolySimd (int idx, uint8_t firstChannel) {
            return inputs [idx].getPolyVoltageSimd<T> (firstChannel);
        }

        template<typename T>
        T getInputNormalSimd (int idx, T normalVoltage, uint8_t firstChannel) {
            return inputs [idx].getNormalVoltageSimd<T> (firstChannel);
        }

        template<typename T>
        T getInputNormalPolySimd (int idx, T normalVoltage, uint8_t firstChannel) {
            return inputs [idx].getNormalPolyVoltageSimd (normalVoltage, firstChannel);
        }

        /*
         * Output setters
         */
        void setOutputChannels (int idx, uint8_t channels) { getOutput (idx).setChannels (channels); }

        void clearOutput (int idx) { outputs [idx].clearVoltages (); }
        void setOutput (int idx, float voltage, uint8_t channel = 0) { outputs [idx].setVoltage (voltage, channel); }
        void writeOutput (int idx, const float* v) { outputs [idx].writeVoltages (v); }

        template<typename T>
        void setOutputSimd (int idx, T voltage, uint8_t firstChannel) {
            outputs [idx].setVoltageSimd (voltage, firstChannel);
        }

        /*
         * Light setters
         */
        void setLight (int idx, float brightness) { lights [idx].setBrightness (brightness); }
        void setLightSmooth (int idx, float brightness, float deltaTime, float lambda = 30.f) {
            lights [idx].setBrightnessSmooth (brightness, deltaTime, lambda);
        }
    };
}