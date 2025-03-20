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

#include "Branch.hpp"

#include "../JsonUtils.hpp"

namespace OuroborosModules {
    rack::plugin::Model* modelBranch = createModel<Modules::Branch::BranchWidget> ("Branch");
}

namespace OuroborosModules::Modules::Branch {
    BranchModule::BranchModule () {
        config (NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);

        for (int i = 0; i < SwitchCount; i++) {
            auto switchText = fmt::format (FMT_STRING ("Destination #{} source"), i + 1);
            configSwitch (PARAM_SWITCH + i, -1.f, 1.f, 0.f, switchText, { "A", "None", "B" });

            auto inputText = fmt::format (FMT_STRING ("Destination #{}"), i + 1);
            configOutput (OUTPUT_DESTINATION + i, inputText);
        }

        configInput (INPUT_A, "A");
        configInput (INPUT_B, "B");
    }

    json_t* BranchModule::dataToJson () {
        auto rootJ = ModuleBase::dataToJson ();

        json_object_set_new_bool (rootJ, "polyOnDemand", polyOnDemand);

        return rootJ;
    }

    void BranchModule::dataFromJson (json_t* rootJ) {
        ModuleBase::dataFromJson (rootJ);

        json_object_try_get_bool (rootJ, "polyOnDemand", polyOnDemand);
    }

    void BranchModule::process (const ProcessArgs& args) {
        using Branchless::ConditionalSet;

        auto maxPolyphony = std::max (std::max (inputs [INPUT_A].getChannels (), inputs [INPUT_B].getChannels ()), 1);

        float voltages [Constants::MaxPolyphony];
        for (int destI = 0; destI < SwitchCount; destI++) {
            auto& curDest = outputs [OUTPUT_DESTINATION + destI];
            auto curSwitchState = static_cast<int> (params [PARAM_SWITCH + destI].getValue ());
            if (!curDest.isConnected ())
                continue;

            auto realChannelCount = 0;
            if (curSwitchState != 0) {
                auto sourceInputId = curSwitchState == -1 ? INPUT_A : INPUT_B;
                inputs [sourceInputId].readVoltages (voltages);
                realChannelCount = inputs [sourceInputId].getChannels ();
            }

            for (int i = realChannelCount; i < Constants::MaxPolyphony; i++)
                voltages [i] = 0;

            curDest.setChannels (!polyOnDemand ? maxPolyphony : realChannelCount);
            curDest.writeVoltages (voltages);
        }
    }
}