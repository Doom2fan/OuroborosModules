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

#include "Bernoulli.hpp"

#include "../JsonUtils.hpp"
#include "../Utils.hpp"

#include <fmt/format.h>

namespace OuroborosModules {
    rack::plugin::Model* modelBernoulli = createModel<Modules::Bernoulli::BernoulliWidget> ("BernoulliGates");
}

namespace OuroborosModules::Modules::Bernoulli {
    BernoulliModule::BernoulliModule () {
        config (PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);

        // Configure parameters.
        for (int i = 0; i < GatesCount; i++) {
            auto probabilityText = fmt::format (FMT_STRING ("Channel {} probability"), i + 1);
            configParam (PARAM_PROBABILITY + i, 0.f, 1.f, .5f, probabilityText, "%", 0, 100);

            auto modeText = fmt::format (FMT_STRING ("Channel {} mode"), i + 1);
            configSwitch (PARAM_MODE + i, 0.f, 3.f, 0.f, modeText, { "Normal", "Toggle", "Latch", "Toggle latch", });

            configInput (INPUT_TRIGGER + i, fmt::format (FMT_STRING ("Channel {} trigger"), i + 1));
            configInput (INPUT_PROBABILITY_CV + i, fmt::format (FMT_STRING ("Channel {} probability"), i + 1));

            auto probabilityCVScaleText = fmt::format (FMT_STRING ("Channel {} probability CV attenuverter"), i + 1);
            configParam (PARAM_PROBABILITY_CV + i, -1.f, 1.f, 0.f, probabilityCVScaleText, "%", 0, 100);

            configOutput (OUTPUT_A + i, fmt::format (FMT_STRING ("Channel {} A"), i + 1));
            configOutput (OUTPUT_B + i, fmt::format (FMT_STRING ("Channel {} B"), i + 1));

            configLight (LIGHT_STATE_A + i, fmt::format (FMT_STRING ("Channel {} A state"), i + 1));
            configLight (LIGHT_STATE_B + i, fmt::format (FMT_STRING ("Channel {} B state"), i + 1));

            bernoulliGates [i] = BernoulliGate ([=] {
                auto cv = inputs [INPUT_PROBABILITY_CV + i].getVoltage () / 10.f
                        * params [PARAM_PROBABILITY_CV + i].getValue ();
                return params [PARAM_PROBABILITY + i].getValue () + cv;
            });
        }

        clockLights.setDivision (32);
    }

    BernoulliGate::BernoulliGate (std::function<float ()> probabilityFunc)
        : schmittTrigger (), probabilityFunc (probabilityFunc) {
        schmittTrigger.reset ();

        modeToggle = false;
        modeLatch = false;

        selectedOutput = false;
    }

    rack::math::Vec BernoulliGate::process (float gateInput) {
        auto isTriggered = schmittTrigger.process (gateInput, Constants::TriggerThreshLow, Constants::TriggerThreshHigh);

        if (isTriggered) {
            auto outcome = rack::random::uniform () < probabilityFunc ();
            if (!modeToggle)
                selectedOutput = outcome;
            else if (outcome)
                selectedOutput ^= true;
        }

        auto outputs = rack::math::Vec (boolToGate (!selectedOutput), boolToGate (selectedOutput));
        if (modeLatch || schmittTrigger.isHigh ())
            return outputs;

        return rack::math::Vec (0.f);
    }

    json_t* BernoulliGate::dataToJson () const {
        auto rootJ = json_object ();

        json_object_set_new_bool (rootJ, "selectedOutput", selectedOutput);
        int schmittTriggerState;
        switch (schmittTrigger.s) {
            default:
            case rack::dsp::SchmittTrigger::UNINITIALIZED: schmittTriggerState = -1; break;
            case rack::dsp::SchmittTrigger::LOW:  schmittTriggerState = 0; break;
            case rack::dsp::SchmittTrigger::HIGH: schmittTriggerState = 1; break;
        }
        json_object_set_new_int (rootJ, "schmittTrigger", schmittTriggerState);

        return rootJ;
    }

    bool BernoulliGate::dataFromJson (json_t* rootJ) {
        if (!json_is_object (rootJ))
            return false;

        json_object_try_get_bool (rootJ, "selectedOutput", selectedOutput);
        int schmittTriggerState = -1;
        json_object_try_get_int (rootJ, "schmittTrigger", schmittTriggerState);
        switch (schmittTriggerState) {
            default:
            case -1: schmittTrigger.s = rack::dsp::SchmittTrigger::UNINITIALIZED; break;
            case  0: schmittTrigger.s = rack::dsp::SchmittTrigger::LOW; break;
            case  1: schmittTrigger.s = rack::dsp::SchmittTrigger::HIGH; break;
        }

        return true;
    }

    std::string getGateJsonName (int i) { return fmt::format (FMT_STRING ("bernoulliGates::{}"), i); }

    json_t* BernoulliModule::dataToJson () {
        auto rootJ = ModuleBase::dataToJson ();

        for (int i = 0; i < GatesCount; i++)
            json_object_set_new_struct (rootJ, getGateJsonName (i).c_str (), bernoulliGates [i]);

        return rootJ;
    }

    void BernoulliModule::dataFromJson (json_t* rootJ) {
        ModuleBase::dataFromJson (rootJ);

        for (int i = 0; i < GatesCount; i++)
            json_object_try_get_struct (rootJ, getGateJsonName (i).c_str (), bernoulliGates [i]);
    }

    void BernoulliModule::process (const ProcessArgs& args) {
        auto lightTime = args.sampleTime * clockLights.division;
        auto lightClocked = clockLights.process ();

        int lastConnected = -1;
        for (int i = 0; i < GatesCount; i++) {
            auto& gate = bernoulliGates [i];

            auto modeValue = static_cast<int> (params [PARAM_MODE + i].getValue ());
            gate.modeLatch = modeValue >= 2;
            gate.modeToggle = (modeValue % 2) == 1;

            float gateInput;
            if (!inputs [INPUT_TRIGGER + i].isConnected ())
                gateInput = inputs [INPUT_TRIGGER + (lastConnected >= 0 ? lastConnected : i)].getVoltage ();
            else {
                gateInput = inputs [INPUT_TRIGGER + i].getVoltage ();
                lastConnected = i;
            }

            auto result = gate.process (gateInput);
            outputs [OUTPUT_A + i].setVoltage (result.x);
            outputs [OUTPUT_B + i].setVoltage (result.y);

            if (lightClocked) {
                lights [LIGHT_STATE_A + i].setSmoothBrightness (result.x, lightTime);
                lights [LIGHT_STATE_B + i].setSmoothBrightness (result.y, lightTime);
            }
        }
    }

    void BernoulliModule::onReset (const ResetEvent& e) {
        ModuleBase::onReset (e);
    }
}