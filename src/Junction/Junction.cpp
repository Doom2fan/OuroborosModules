/*
 *  OuroborosModules
 *  Copyright (C) 2024 Chronos "phantombeta" Ouroboros
 *  Copyright (C) 2016-2023 VCV
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

#include "Junction.hpp"

#include "../JsonUtils.hpp"

namespace OuroborosModules {
    rack::plugin::Model* modelJunction = createModel<Modules::Junction::JunctionWidget> ("Junction");
}

namespace OuroborosModules::Modules::Junction {
    JunctionModule::JunctionModule () {
        config (NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);

        for (int i = 0; i < SwitchCount; i++) {
            auto switchText = fmt::format (FMT_STRING ("Signal #{} destination"), i + 1);
            configSwitch (PARAM_SWITCH + i, -1.f, 1.f, 0.f, switchText, { "A", "None", "B" });

            auto inputText = fmt::format (FMT_STRING ("Signal #{}"), i + 1);
            configInput (INPUT_SIGNAL + i, inputText);
        }

        configOutput (OUTPUT_SIGNAL    , "A");
        configOutput (OUTPUT_SIGNAL + 1, "B");
    }

    json_t* JunctionModule::dataToJson () {
        auto rootJ = ModuleBase::dataToJson ();

        json_object_set_new_bool (rootJ, "polyOnDemand", polyOnDemand);
        json_object_set_new_bool (rootJ, "clampWhileSumming", clampWhileSumming);

        return rootJ;
    }

    void JunctionModule::dataFromJson (json_t* rootJ) {
        ModuleBase::dataFromJson (rootJ);

        json_object_try_get_bool (rootJ, "polyOnDemand", polyOnDemand);
        json_object_try_get_bool (rootJ, "clampWhileSumming", clampWhileSumming);
    }

    struct OutputData {
        uint8_t inputs [JunctionModule::SwitchCount];
        uint8_t inputCount = 0;

        void addInput (uint8_t id) { inputs [inputCount++] = id; }
    };

    void JunctionModule::process (const ProcessArgs& args) {
        using rack::simd::float_4;
        using Branchless::ConditionalSet;

        OutputData outputData [OutputCount];
        int inputMaxPolyphony = 1;

        for (uint8_t signalI = 0; signalI < SwitchCount; signalI++) {
            if (!inputs [INPUT_SIGNAL + signalI].isConnected ())
                continue;

            auto curSwitchState = std::clamp (static_cast<int> (params [PARAM_SWITCH + signalI].getValue ()), -1, 1);
            inputMaxPolyphony = std::max (inputMaxPolyphony, inputs [INPUT_SIGNAL + signalI].getChannels ());
            if (curSwitchState != 0)
                outputData [(curSwitchState < 0) ? 0 : 1].addInput (signalI);
        }

        if (!polyOnDemand) {
            for (int i = 0; i < OutputCount; i++)
                outputs [OUTPUT_SIGNAL + i].setChannels (inputMaxPolyphony);
        } else {
            for (int outputI = 0; outputI < OutputCount; outputI++) {
                auto& curOutput = outputData [outputI];

                int polyphonyCount = 1;
                for (int inputI = 0; inputI < curOutput.inputCount; inputI++)
                    polyphonyCount = std::max (polyphonyCount, inputs [INPUT_SIGNAL + curOutput.inputs [inputI]].getChannels ());

                outputs [OUTPUT_SIGNAL + outputI].setChannels (polyphonyCount);
            }
        }

        auto voltageMax = float_4 (10);
        auto voltageMin = -voltageMax;

        for (int outputI = 0; outputI < OutputCount; outputI++) {
            auto& curOutput = outputData [outputI];
            float_4 voltages [SIMDBankCount] = {};

            for (int inputI = 0; inputI < curOutput.inputCount; inputI++) {
                auto inputIdx = curOutput.inputs [inputI];
                auto inputChannelCount = inputs [inputIdx].getChannels ();

                auto doClamp = clampWhileSumming ? true : (inputI == curOutput.inputCount - 1);
                for (int bankI = 0; bankI < SIMDBankCount; bankI++) {
                    auto curChannel = bankI * SIMDBankSize;
                    auto curBank = inputs [inputIdx].getVoltageSimd<float_4> (curChannel);
                    curBank &= (float_4 (0, 1, 2, 3) + curChannel) < inputChannelCount;

                    voltages [bankI] += curBank;
                    ConditionalSet (voltages [bankI], doClamp, rack::simd::clamp (voltages [bankI], voltageMin, voltageMax));
                }
            }

            for (int i = 0; i < SIMDBankCount; i++)
                outputs [OUTPUT_SIGNAL + outputI].setVoltageSimd (voltages [i], i * SIMDBankSize);
        }
    }
}