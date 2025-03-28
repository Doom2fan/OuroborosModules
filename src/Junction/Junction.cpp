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

        configOutput (OUTPUT_A, "A");
        configOutput (OUTPUT_B, "B");
    }

    json_t* JunctionModule::dataToJson () {
        auto rootJ = ModuleBase::dataToJson ();

        json_object_set_new_bool (rootJ, "polyOnDemand", polyOnDemand);

        return rootJ;
    }

    void JunctionModule::dataFromJson (json_t* rootJ) {
        ModuleBase::dataFromJson (rootJ);

        json_object_try_get_bool (rootJ, "polyOnDemand", polyOnDemand);
    }

    void JunctionModule::process (const ProcessArgs& args) {
        using rack::simd::float_4;
        using Branchless::ConditionalSet;

        float_4 outputA [SIMDBankCount] = {};
        float_4 outputB [SIMDBankCount] = {};
        int polyphonyCountA = 1;
        int polyphonyCountB = 1;
        int inputMaxPolyphony = 1;

        for (int signalI = 0; signalI < SwitchCount; signalI++) {
            auto& curInput = inputs [INPUT_SIGNAL + signalI];
            if (!curInput.isConnected ())
                continue;

            auto curSwitchState = std::clamp (static_cast<int> (params [PARAM_SWITCH + signalI].getValue ()), -1, 1);
            auto curChannelCount = curInput.getChannels ();
            inputMaxPolyphony = std::max (inputMaxPolyphony, curChannelCount);

            polyphonyCountA = std::max (polyphonyCountA, curSwitchState == -1 ? curChannelCount : 0);
            polyphonyCountB = std::max (polyphonyCountB, curSwitchState ==  1 ? curChannelCount : 0);
            for (int bankI = 0; bankI < SIMDBankCount; bankI++) {
                auto curBank = curInput.getVoltageSimd<float_4> (bankI * SIMDBankSize);

                outputA [bankI] += (curSwitchState == -1 ? curBank : float_4::zero ());
                outputB [bankI] += (curSwitchState ==  1 ? curBank : float_4::zero ());
            }
        }

        ConditionalSet (polyphonyCountA, !polyOnDemand, inputMaxPolyphony);
        ConditionalSet (polyphonyCountB, !polyOnDemand, inputMaxPolyphony);

        auto voltageMax = float_4 (10);
        auto voltageMin = -voltageMax;
        for (int bankI = 0; bankI < SIMDBankCount; bankI++) {
            outputA [bankI] = rack::simd::clamp (outputA [bankI], voltageMin, voltageMax);
            outputB [bankI] = rack::simd::clamp (outputB [bankI], voltageMin, voltageMax);
        }

        outputs [OUTPUT_A].setChannels (polyphonyCountA);
        outputs [OUTPUT_B].setChannels (polyphonyCountB);

        for (int i = 0; i < SIMDBankCount; i++) {
            outputs [OUTPUT_A].setVoltageSimd (outputA [i], i * SIMDBankSize);
            outputs [OUTPUT_B].setVoltageSimd (outputB [i], i * SIMDBankSize);
        }
    }
}