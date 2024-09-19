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

#include "STVCA.hpp"

#include "../JsonUtils.hpp"

namespace OuroborosModules {
    rack::plugin::Model* modelSTVCA = createModel<Modules::STVCA::STVCAWidget> ("StereoVCAModule");
}

namespace OuroborosModules {
namespace Modules {
namespace STVCA {
    STVCAModule::STVCAModule () {
        config (PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);

        // Configure parameters.
        configParam (PARAM_LEVEL, 0.f, 1.f, 1.f, "Level", "%", 0, 100);
        configSwitch (PARAM_EXP, 0.f, 1.f, 1.f, "Response mode", { "Exponential", "Linear", });

        // Configure inputs and outputs.
        configInput (INPUT_LEFT, "Left");
        configInput (INPUT_RIGHT, "Right");
        configInput (INPUT_CV, "CV");

        configOutput (OUTPUT_LEFT, "Left");
        configOutput (OUTPUT_RIGHT, "Right");

        // Configure bypasses.
        configBypass (INPUT_LEFT, OUTPUT_LEFT);
        configBypass (INPUT_RIGHT, OUTPUT_RIGHT);
    }

    json_t* STVCAModule::dataToJson () {
        auto rootJ = ModuleBase::dataToJson ();

        json_object_set_new_bool (rootJ, "displayColor::UseDefault", displayColorUseDefault);
        json_object_set_new_struct (rootJ, "displayColor", displayColor);

        return rootJ;
    }

    void STVCAModule::dataFromJson (json_t* rootJ) {
        ModuleBase::dataFromJson (rootJ);

        json_object_try_get_bool (rootJ, "displayColor::UseDefault", displayColorUseDefault);
        json_object_try_get_struct (rootJ, "displayColor", displayColor);
    }

    void STVCAModule::process (const ProcessArgs& args) {
        auto channels = std::max ({
            1,
            inputs [INPUT_LEFT].getChannels (),
            inputs [INPUT_RIGHT].getChannels (),
            inputs [INPUT_CV].getChannels (),
        });
        auto level = params [PARAM_LEVEL].getValue ();

        for (int c = 0; c < channels; c++) {
            // Calculate gain.
            auto gain = level;
            if (inputs [INPUT_CV].isConnected ()) {
                float cv = std::clamp (inputs [INPUT_CV].getPolyVoltage (c) / 10.f, 0.f, 1.f);

                if (int (params [PARAM_EXP].getValue ()) == 0)
                    cv = std::pow (cv, 4.f);

                gain *= cv;
            }

            // Get inputs.
            auto inL = inputs [INPUT_LEFT].getPolyVoltage (c);
            auto inR = inputs [INPUT_RIGHT].getPolyVoltage (c);

            // Apply gain.
            inL *= gain;
            inR *= gain;
            lastGains [c] = gain;

            // Set outputs.
            outputs [OUTPUT_LEFT].setVoltage (inL, c);
            outputs [OUTPUT_RIGHT].setVoltage (inR, c);
        }

        outputs [OUTPUT_LEFT].setChannels (channels);
        outputs [OUTPUT_RIGHT].setChannels (channels);
        lastChannels = channels;
    }

    void STVCAModule::onReset (const ResetEvent& e) {
        ModuleBase::onReset (e);

        displayColorUseDefault = true;
        displayColor = RGBColor ();
    }
}
}
}