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

#include "ResetHelper.hpp"

#include <fmt/format.h>

namespace OuroborosModules {
    rack::plugin::Model* modelResetHelper = createModel<Modules::ResetHelper::ResetHelperWidget> ("ResetHelper");
}

namespace OuroborosModules::Modules::ResetHelper {
    ResetHelperModule::ResetHelperModule () {
        config (NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);

        // Configure params, I/O and lights.
        configParam (PARAM_MERGE_TIME, 0, 1000, 25, "Merge window", " ms");
        configParam (PARAM_TRIGGER_LENGTH, 1, 1000, 10, "Reset trigger length", " ms");

        for (int i = 0; i < InputCount; i++) {
            configInput (INPUT_RESET + i, fmt::format (FMT_STRING ("Gate/Trigger #{}"), i + 1));
            configInput (INPUT_RESET + i, fmt::format (FMT_STRING ("Gate/Trigger #{}"), i + 1));
        }

        configOutput (OUTPUT_RESET, "Reset trigger");
        configLight (LIGHT_OUTPUT, "Reset trigger");

        // Configure clock dividers.
        clockLights = DSP::ClockDivider (32, rack::random::u32 ());
    }

    void ResetHelperModule::process (const ProcessArgs& args) {
        // Fetch and calculate the parameters.
        static constexpr auto lightPulseLength = 10.f / 1000.f;
        auto mergeTime = params [PARAM_MERGE_TIME].getValue () / 1000.f;
        auto triggerLength = params [PARAM_TRIGGER_LENGTH].getValue () / 1000.f;

        mergeTime = std::max (mergeTime, triggerLength); // This ensures more consistent behaviour from the module.

        // Handle light clocking.
        auto lightTime = args.sampleTime * clockLights.division;
        auto lightsClocked = clockLights.process ();

        // Determine and set the channel counts.
        auto outputChannels = std::max (1, inputs [INPUT_RESET].getChannels ());
        for (int i = 1; i < InputCount; i++)
            outputChannels = std::max (outputChannels, inputs [INPUT_RESET + i].getChannels ());

        outputs [OUTPUT_RESET].setChannels (outputChannels);

        // Light data.
        bool inputTriggered [InputCount] = { };
        bool outputTriggered = false;

        // Process the channels.
        for (int channel = 0; channel < outputChannels; channel++) {
            // Check for rising edges on the inputs.
            auto triggered = false;

            for (int inputIdx = 0; inputIdx < InputCount; inputIdx++) {
                auto inputSignal = inputs [INPUT_RESET + inputIdx].getPolyVoltage (channel);
                auto inputHigh = inputTriggers [inputIdx] [channel].process (inputSignal, Constants::TriggerThreshLow, Constants::TriggerThreshHigh);

                triggered |= inputHigh;
                inputTriggered [inputIdx] |= inputHigh;
            }

            // Determine if we can trigger.
            auto canTrigger = !mergeWindowPulse [channel].process (args.sampleTime);
            auto doTrigger = triggered & canTrigger;

            // Trigger the output and the merge window pulses.
            if (doTrigger) {
                outputPulse [channel].trigger (triggerLength);
                mergeWindowPulse [channel].trigger (mergeTime);

                outputTriggered = true;
            }

            // Update the reset output.
            auto outputHigh = outputPulse [channel].process (args.sampleTime);
            outputs [OUTPUT_RESET].setVoltage (boolToGate (outputHigh), channel);
        }

        // Pulse the lights, and store the pulse state.
        for (int inputIdx = 0; inputIdx < InputCount; inputIdx++) {
            if (inputTriggered [inputIdx])
                inputLightPulses [inputIdx].trigger (lightPulseLength);
            inputTriggered [inputIdx] = inputLightPulses [inputIdx].process (args.sampleTime);
        }
        if (outputTriggered)
            outputLightPulse.trigger (lightPulseLength);
        outputTriggered = outputLightPulse.process (args.sampleTime);

        // Handle the lights.
        if (lightsClocked) {
            for (int inputIdx = 0, lightIdx = LIGHT_INPUT; inputIdx < InputCount; inputIdx++, lightIdx += 2) {
                auto polyIn = inputs [INPUT_RESET + inputIdx].getChannels () > 1;
                auto inputLightState = inputTriggered [inputIdx];

                lights [lightIdx    ].setBrightnessSmooth (boolToLight (!polyIn & inputLightState), lightTime);
                lights [lightIdx + 1].setBrightnessSmooth (boolToLight ( polyIn & inputLightState), lightTime);
            }

            auto polyOut = outputChannels > 1;
            auto outputLightState = outputTriggered;
            lights [LIGHT_OUTPUT    ].setBrightnessSmooth (boolToLight (!polyOut & outputLightState), lightTime);
            lights [LIGHT_OUTPUT + 1].setBrightnessSmooth (boolToLight ( polyOut & outputLightState), lightTime);
        }
    }
}