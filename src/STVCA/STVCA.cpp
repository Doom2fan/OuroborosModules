/*
 *  OuroborosModules
 *  Copyright (C) 2024-2025 Chronos "phantombeta" Ouroboros
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

namespace OuroborosModules::Modules::STVCA {
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
            getInputChannels (INPUT_LEFT),
            getInputChannels (INPUT_RIGHT),
            getInputChannels (INPUT_CV),
        });
        auto level = getParam (PARAM_LEVEL);

        for (int c = 0; c < channels; c++) {
            // Calculate gain.
            auto gain = level;
            if (isInputConnected (INPUT_CV)) {
                float cv = std::clamp (getInputPoly (INPUT_CV, c) / 10.f, 0.f, 1.f);

                if (int (getParam (PARAM_EXP)) == 0)
                    cv = std::pow (cv, 4.f);

                gain *= cv;
            }

            // Get inputs.
            auto inL = getInputPoly (INPUT_LEFT, c);
            auto inR = getInputPoly (INPUT_RIGHT, c);

            // Apply gain.
            inL *= gain;
            inR *= gain;
            lastGains [c] = gain;

            // Set outputs.
            setOutput (OUTPUT_LEFT, inL, c);
            setOutput (OUTPUT_RIGHT, inR, c);
        }

        setOutputChannels (OUTPUT_LEFT, channels);
        setOutputChannels (OUTPUT_RIGHT, channels);
        lastChannels = channels;
    }

    void STVCAModule::onReset (const ResetEvent& e) {
        ModuleBase::onReset (e);

        displayColorUseDefault = true;
        displayColor = RGBColor ();
    }
}