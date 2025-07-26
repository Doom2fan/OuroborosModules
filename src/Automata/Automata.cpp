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

#include "Automata.hpp"

#include "../JsonUtils.hpp"
#include "../Utils.hpp"

#include <fmt/format.h>

namespace OuroborosModules {
    rack::plugin::Model* modelAutomata = createModel<Modules::Automata::AutomataWidget> ("Automata");
}

namespace OuroborosModules::Modules::Automata {
    AutomataModule::AutomataModule () {
        config (PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);

        // Configure parameters and I/O.
        configButton (PARAM_STEP_BUTTON, "Step sequence");
        configButton (PARAM_RESET_BUTTON, "Reset");
        configInput (INPUT_CLOCK, "Clock");
        configInput (INPUT_RESET, "Reset trigger");

        configButton (PARAM_RANDOMIZE_BUTTON, "Randomize");
        configInput (INPUT_RANDOMIZE, "Randomize trigger");
        configParam (PARAM_RANDOM_DENSITY, 0.f, 1.f, .25f, "Random density", "%", 0, 100);
        configInput (INPUT_RANDOM_DENSITY_CV, "Random density CV");
        configParam (PARAM_RANDOM_DENSITY_CV_ATTENUVERTER, -1.f, 1.f, 0.f, "Random density CV attenuverter", "%", 0, 100);

        configButton (PARAM_LENGTH_BUTTON, "Automatic reset");
        configParamSnap (PARAM_LENGTH, 1.f, MaxSequenceLength, 16.f, "Automatic reset length");
        configInput (INPUT_LENGTH_ENABLE, "Automatic reset toggle");
        configInput (INPUT_LENGTH_CV, "Automatic reset length CV");
        configParam (PARAM_LENGTH_CV_ATTENUVERTER, -1.f, 1.f, 0.f, "Automatic reset length CV attenuverter", "%", 0, 100);

        auto modeSelectLabels = std::vector<std::string> ();
        modeSelectLabels.reserve (TriggerCount + 2);
        modeSelectLabels.push_back ("Play");
        modeSelectLabels.push_back ("Edit seed");
        for (int i = 0; i < TriggerCount; i++)
            modeSelectLabels.push_back (fmt::format (FMT_STRING ("Edit trigger set {}"), i + 1));
        configSwitch (PARAM_MODE_SELECT, -2, TriggerCount - 1, -2, "Mode select", modeSelectLabels);

        for (int i = 0; i < TriggerCount; i++)
            configOutput (OUTPUT_TRIGGER + i, fmt::format (FMT_STRING ("Trigger set {}"), i + 1));
        configOutput (OUTPUT_EOC, "End of Cycle");

        // Disable randomization for relevant params.
        getParamQuantity (PARAM_MODE_SELECT)->randomizeEnabled = false;

        initialize ();

        // Clock dividers.
        clockLights = DSP::ClockDivider (32, rack::random::u32 ());
        clockParams = DSP::ClockDivider (32, rack::random::u32 ());

        // Schmitt triggers
        stepButtonTrigger = rack::dsp::SchmittTrigger ();
        clockTrigger = rack::dsp::SchmittTrigger ();

        resetButtonTrigger = rack::dsp::SchmittTrigger ();
        resetTrigger = rack::dsp::SchmittTrigger ();

        randomizeButtonTrigger = rack::dsp::SchmittTrigger ();
        randomizeTrigger = rack::dsp::SchmittTrigger ();

        lengthButtonTrigger = rack::dsp::SchmittTrigger ();
        lengthEnableTrigger = rack::dsp::SchmittTrigger ();

        // Widget communication
        updateRulesSignal.store (false);
    }

    void AutomataModule::initialize () {
        commandQueue.clear ();

        // Initialize state.
        currentMode = AutomataMode::Play;
        lengthEnabled = false;
        stepCount = 0;

        // Initialize options.
        randomizeOnManualReset = false;
        randomizeOnAutoReset = false;
        momentaryLengthEnable = false;
    }

    void AutomataModule::process (const ProcessArgs& args) {
        using Constants::TriggerThreshLow;
        using Constants::TriggerThreshHigh;

        auto lightTime = args.sampleTime * clockLights.division;
        auto lightsClocked = clockLights.process ();
        auto paramsClocked = clockParams.process ();

        while (!commandQueue.empty ()) {
            auto [isUndo, cmd] = commandQueue.consume ();
            if (!isUndo)
                cmd->execute (lifeBoard.getBoard ());
            else
                cmd->undo (lifeBoard.getBoard ());

            cmd->executed = true;
        }

        auto newRules = this->newRules;
        if (updateRulesSignal.exchange (false))
            lifeBoard.setRules (newRules);

        // Handle params.
        if (paramsClocked)
            currentMode = modeFromSelectorParam (params [PARAM_MODE_SELECT].getValue ());

        // Handle reset.
        auto resetPulseHigh = resetPulse.process (args.sampleTime);
        if (resetButtonTrigger.process (params [PARAM_RESET_BUTTON].getValue ()) |
            resetTrigger.process (inputs [INPUT_RESET].getVoltage (), TriggerThreshLow, TriggerThreshHigh)) {
            processReset (false);

            lights [LIGHT_RESET_BUTTON].setBrightness (1);
        } else if (lightsClocked)
            lights [LIGHT_RESET_BUTTON].setBrightnessSmooth (0.f, lightTime);

        // Handle randomize.
        if (randomizeButtonTrigger.process (params [PARAM_RANDOMIZE_BUTTON].getValue ()) |
            randomizeTrigger.process (inputs [INPUT_RANDOMIZE].getVoltage (), TriggerThreshLow, TriggerThreshHigh)) {
            processRandomize ();

            lights [LIGHT_RANDOMIZE_BUTTON].setBrightness (1);
        } else if (lightsClocked)
            lights [LIGHT_RANDOMIZE_BUTTON].setBrightnessSmooth (0.f, lightTime);

        // Handle length toggle.
        auto lengthButtonPressed = lengthButtonTrigger.process (params [PARAM_LENGTH_BUTTON].getValue ());
        auto lengthEnableRise = lengthEnableTrigger.process (inputs [INPUT_LENGTH_ENABLE].getVoltage (), TriggerThreshLow, TriggerThreshHigh);
        if (momentaryLengthEnable && inputs [INPUT_LENGTH_ENABLE].isConnected ())
            lengthEnabled = inputs [INPUT_LENGTH_ENABLE].getVoltage () >= TriggerThreshHigh;
        else if (lengthButtonPressed || lengthEnableRise)
            lengthEnabled = !lengthEnabled;
        stepCount = lengthEnabled ? stepCount : 0;

        if (lightsClocked)
            lights [LIGHT_LENGTH_BUTTON].setBrightnessSmooth (boolToLight (lengthEnabled), lightTime);

        // Handle step/clock.
        auto doStep = stepButtonTrigger.process (params [PARAM_STEP_BUTTON].getValue ())
                    | clockTrigger.process (inputs [INPUT_CLOCK].getVoltage (), TriggerThreshLow, TriggerThreshHigh);
        auto didStep = false;
        auto eocReset = false;
        if (doStep && !resetPulseHigh) {
            if (lengthEnabled) {
                auto seqLength = params [PARAM_LENGTH].getValue ();
                seqLength += inputs [INPUT_LENGTH_CV].getVoltage () / 10.f
                           * params [PARAM_LENGTH_CV_ATTENUVERTER].getValue ()
                           * MaxSequenceLength;
                seqLength = std::clamp (seqLength, 1.f, static_cast<float> (MaxSequenceLength));
                eocReset = ++stepCount >= static_cast<int> (seqLength);
            }

            processStep (args);
            didStep = true;

            lights [LIGHT_STEP_BUTTON].setBrightness (1);
        } else if (lightsClocked)
            lights [LIGHT_STEP_BUTTON].setBrightnessSmooth (0.f, lightTime);

        // Process outputs.
        if (didStep)
            processTriggers ();

        if (eocReset)
            processReset (true);

        for (int i = 0; i < TriggerCount; i++) {
            float pulseHigh = outputPulses [i].process (args.sampleTime);

            float outputValue;
            switch (triggerInfo [i].outputMode) {
                default:
                case AutomataTriggerOutputMode::Trigger: outputValue = boolToGate (pulseHigh); break;
                case AutomataTriggerOutputMode::Percentage: outputValue = outputValues [i]; break;
            }

            outputs [OUTPUT_TRIGGER + i].setVoltage (outputValue);
        }

        outputs [OUTPUT_EOC].setVoltage (boolToGate (eocPulse.process (args.sampleTime)));

        lifeBoard.handleUpdated ();
    }

    void AutomataModule::processReset (bool automatic) {
        if ((automatic && randomizeOnAutoReset) || (!automatic && randomizeOnManualReset)) {
            processRandomize ();
        } else
            lifeBoard.reset ();

        stepCount = 0;
        resetPulse.trigger ();
        if (automatic)
            eocPulse.trigger ();
    }

    void AutomataModule::processRandomize () {
        auto density = params [PARAM_RANDOM_DENSITY].getValue ();
        density += inputs [INPUT_RANDOM_DENSITY_CV].getVoltage () / 10.f
                 * params [PARAM_RANDOM_DENSITY_CV_ATTENUVERTER].getValue ();
        density = std::clamp (density, 0.f, 1.f);

        lifeBoard.randomize (density);
    }

    void AutomataModule::processStep (const ProcessArgs& args) {
        lifeBoard.process ();
    }

    void AutomataModule::processTriggers () {
        using rack::simd::float_4;

        static constexpr int SIMDBankSize = 4;
        static constexpr int SIMDBankCount = static_cast<int> (static_cast<float> (TriggerCount) / SIMDBankSize + .5f);

        float_4 triggerLocationsCount [SIMDBankCount];
        float_4 triggerCounts [SIMDBankCount];
        float_4 triggerNewbornMask [SIMDBankCount];
        float_4 triggerAliveMask [SIMDBankCount];

        for (int i = 0, bankIdx = 0; bankIdx < SIMDBankCount; i += SIMDBankSize, bankIdx++) {
            auto maxBankIdx = std::min (i + SIMDBankSize, TriggerCount);

            triggerLocationsCount [bankIdx] = float_4::zero ();
            triggerCounts [bankIdx] = float_4::zero ();

            int newbornMaskMoveMask = 0;
            int aliveMaskMoveMask = 0;
            for (int j = i; j < maxBankIdx; j++) {
                const auto& curTriggerInfo = triggerInfo [j];
                int laneMask = 1 << (j - i);

                newbornMaskMoveMask |= curTriggerInfo.countMode == AutomataTriggerCountMode::Newborn ? laneMask : 0;
                aliveMaskMoveMask |= curTriggerInfo.countMode == AutomataTriggerCountMode::IsAlive ? laneMask : 0;
            }

            triggerNewbornMask [bankIdx] = rack::simd::movemaskInverse<float_4> (newbornMaskMoveMask);
            triggerAliveMask [bankIdx] = rack::simd::movemaskInverse<float_4> (aliveMaskMoveMask);
        }

        const auto& board = lifeBoard.getBoard ();
        auto liveFlag = board.getLiveFlag ();
        auto prevLiveFlag = board.getLiveFlagPrev ();

        for (int y = 0; y < BoardHeight; y++) {
            for (int x = 0; x < BoardWidth; x++) {
                const auto& cell = board.at (x, y);

                auto isAlive = testCellFlag (cell, liveFlag);
                auto newborn = !testCellFlag (cell, prevLiveFlag) & isAlive;
                auto triggers = extractCellTriggers (cell);

                for (int bankIdx = 0; bankIdx < SIMDBankCount; bankIdx++) {
                    auto triggersMask = rack::simd::movemaskInverse<float_4> (triggers >> (bankIdx * SIMDBankSize));
                    triggerCounts [bankIdx] += (triggerAliveMask [bankIdx] & triggersMask & float_4 (isAlive ? 1 : 0))
                                            +  (triggerNewbornMask [bankIdx] & triggersMask & float_4 (newborn ? 1 : 0));

                    triggerLocationsCount [bankIdx] += triggersMask & float_4 (1);
                }
            }
        }

        for (int bankIdx = 0, i = 0; bankIdx < SIMDBankCount; bankIdx++, i += SIMDBankSize) {
            auto maxBankIdx = std::min (i + SIMDBankSize, TriggerCount);

            float percentageBank [SIMDBankSize];
            float hitsCountBank [SIMDBankSize];

            (triggerCounts [bankIdx] / rack::simd::fmax (triggerLocationsCount [bankIdx], float_4 (1))).store (percentageBank);
            triggerCounts [bankIdx].store (hitsCountBank);

            for (int j = i; j < maxBankIdx; j++) {
                auto laneIdx = j - i;
                if (hitsCountBank [laneIdx] > 0)
                    outputPulses [j].trigger ();
                outputValues [j] = percentageBank [laneIdx] * 10.f;
            }
        }
    }

    void AutomataModule::onReset (const ResetEvent& e) {
        ModuleBase::onReset (e);
        lifeBoard.initialize ();
        initialize ();
    }

    json_t* AutomataModule::dataToJson () {
        auto rootJ = ModuleBase::dataToJson ();

        json_object_set_new_struct (rootJ, "boardData", lifeBoard);
        json_object_set_new_bool (rootJ, "lengthEnabled", lengthEnabled);
        json_object_set_new_bool (rootJ, "randomizeOnManualReset", randomizeOnManualReset);
        json_object_set_new_bool (rootJ, "randomizeOnAutoReset", randomizeOnAutoReset);
        json_object_set_new_bool (rootJ, "momentaryLengthEnable", momentaryLengthEnable);
        json_object_set_new_int (rootJ, "stepCount", stepCount);

        auto triggerInfoJ = json_array ();
        for (int i = 0; i < TriggerCount; i++)
            json_array_append_new (triggerInfoJ, triggerInfo [i].dataToJson ());
        json_object_set_new (rootJ, "triggerInfo", triggerInfoJ);

        return rootJ;
    }

    void AutomataModule::dataFromJson (json_t* rootJ) {
        ModuleBase::dataFromJson (rootJ);

        commandQueue.clear ();

        json_object_try_get_struct (rootJ, "boardData", lifeBoard);
        json_object_try_get_bool (rootJ, "lengthEnabled", lengthEnabled);
        json_object_try_get_bool (rootJ, "randomizeOnManualReset", randomizeOnManualReset);
        json_object_try_get_bool (rootJ, "randomizeOnAutoReset", randomizeOnAutoReset);
        json_object_try_get_bool (rootJ, "momentaryLengthEnable", momentaryLengthEnable);
        json_object_try_get_int (rootJ, "stepCount", stepCount);

        auto triggerInfoJ = json_object_get (rootJ, "triggerInfo");
        if (json_is_array (triggerInfoJ)) {
            auto arrayLength = static_cast<int> (json_array_size (triggerInfoJ));
            for (int i = 0; i < TriggerCount; i++) {
                auto triggerInfoElementJ = json_array_get (triggerInfoJ, i);

                if (i >= arrayLength || !triggerInfo [i].dataFromJson (triggerInfoElementJ))
                    triggerInfo [i] = AutomataTriggerInfo ();
            }
        }
    }
}