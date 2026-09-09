/*
 *  OuroborosModules
 *  Copyright (C) 2026 Chronos "phantombeta" Ouroboros
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

#include "Conductor.hpp"

#include "../JsonUtils.hpp"

#include <fmt/format.h>

namespace OuroborosModules {
    rack::plugin::Model* modelConductor = createModel<Modules::Conductor::ConductorWidget> ("Conductor");
}

namespace OuroborosModules::Modules::Conductor {
    struct PatternCountQuantity : rack::engine::ParamQuantity {
        float getDisplayValue () override {
            auto module = reinterpret_cast<ConductorModule*> (this->module);
            if (module != nullptr) {
                displayMultiplier = module->curSequencerMax - 2;
                displayOffset = 2;
            }

            return patternFloatToInt (ParamQuantity::getDisplayValue ());
        }
    };
    struct ConductorPatternIndexQuantity : PatternIndexQuantity {
        int getPatternCount () override {
            auto module = reinterpret_cast<ConductorModule*> (this->module);
            return (module != nullptr) ? module->curPatternCount : 0;
        }
    };


    void ConductorModule::ClockHandler::setParams (bool ignoreFirstClock, uint32_t delay) {
        assert (delay <= MaxDelay);
        if (delay > MaxDelay)
            delay = MaxDelay;

        this->ignoreFirstClock = ignoreFirstClock;
        delayTime = delay;
    }

    void ConductorModule::ClockHandler::resetPulse () {
        if (ignoreFirstClock)
            resetPhasor = 1.f / 1000.f; // 1ms
    }

    bool ConductorModule::ClockHandler::processClock (const ProcessArgs& args, float clockInput) {
        using Constants::TriggerThreshLow;
        using Constants::TriggerThreshHigh;
        using rack::dsp::SchmittTrigger;

        clockDelayLineIndex = (clockDelayLineIndex + 1) % (MaxDelay + 1);
        clockDelayLine [clockDelayLineIndex] = clockInput;

        auto clockLine = clockDelayLine [(clockDelayLineIndex - delayTime + (MaxDelay + 1)) % (MaxDelay + 1)];
        auto clockEvent = clockTrigger.processEvent (clockLine, TriggerThreshLow, TriggerThreshHigh);

        if (clockEvent == SchmittTrigger::TRIGGERED) {
            if (resetPhasor < 1e-5)
                clockHigh = true;
            else
                resetPhasor = 0;
        } else if (clockEvent == SchmittTrigger::UNTRIGGERED)
            clockHigh = false;

        resetPhasor = std::max (resetPhasor - args.sampleTime, 0.f);

        return clockHigh;
    }

    ConductorModule::ConductorModule () {
        config (PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);

        // Configure params.
        configParamSnap (PARAM_SEQ_MAX_PATTERNS, 2, MaxPatterns, 16, "Sequencer's pattern count");
        configParam (PARAM_SEQ_MAX_CV, 0.f, 10.f, 10.f, "Sequencer's max pattern CV");

        configParam<PatternCountQuantity> (PARAM_PATTERN_COUNT, 0.f, 1.f, 1.f, "Max pattern count");

        configParam<ConductorPatternIndexQuantity> (PARAM_RESET_PATTERN, 0.f, 1.f, 0.f, "Reset pattern");
        configButton (PARAM_RESET_PATTERN_BUTTON, "Reset pattern toggle");

        configButton (PARAM_MANUAL_RESET_BUTTON, "Manual reset");
        configButton (PARAM_MANUAL_ADVANCE_BUTTON, "Manual advance");

        configParam<ConductorPatternIndexQuantity> (PARAM_MANUAL_SET, 0.f, 1.f, 0.f, "Set pattern index");
        configButton (PARAM_MANUAL_SET_BUTTON, "Set pattern");

        configParam (PARAM_PATTERN_OFFSET_VOLTAGE, 0.f, 1.f, .5f, "Pattern output offset", "%", 0.f, 100.f);

        configParam (PARAM_CLOCK_DELAY, 0.f, ClockHandler::MaxDelay, 2.f, "Clock delay", " samples", 0.f, 1.f);
        getParamQuantity (PARAM_CLOCK_DELAY)->snapEnabled = true;

        // Disable randomization for relevant params.
        getParamQuantity (PARAM_SEQ_MAX_PATTERNS)->randomizeEnabled = false;
        getParamQuantity (PARAM_SEQ_MAX_CV)->randomizeEnabled = false;
        getParamQuantity (PARAM_MANUAL_SET)->randomizeEnabled = false;
        getParamQuantity (PARAM_PATTERN_OFFSET_VOLTAGE)->randomizeEnabled = false;
        getParamQuantity (PARAM_CLOCK_DELAY)->randomizeEnabled = false;

        // Configure inputs.
        configInput (INPUT_CLOCK, "Clock");
        configInput (INPUT_ADVANCE, "Advance trigger");
        configInput (INPUT_RESET, "Reset trigger");

        // Configure outputs.
        configOutput (OUTPUT_CLOCK, "Clock");
        configOutput (OUTPUT_RESET, "Reset trigger");
        configOutput (OUTPUT_PATTERN, "Sequencer pattern CV");

        // Configure bypasses.
        configBypass (INPUT_CLOCK, OUTPUT_CLOCK);
        configBypass (INPUT_RESET, OUTPUT_RESET);

        // Schmitt triggers.
        advanceTrigger = rack::dsp::SchmittTrigger ();
        resetTrigger = rack::dsp::SchmittTrigger ();

        advanceButtonTrigger = rack::dsp::SchmittTrigger ();
        resetButtonTrigger = rack::dsp::SchmittTrigger ();
        resetPatternToggleTrigger = rack::dsp::SchmittTrigger ();
        patternSetButtonTrigger = rack::dsp::SchmittTrigger ();
    }

    void ConductorModule::onSampleRateChange (const SampleRateChangeEvent& e) {
        ModuleBase::onSampleRateChange (e);

        // Clock dividers.
        clockLights = DSP::ClockDivider (static_cast<uint32_t> (e.sampleRate / (48000.f / 128)), rack::random::u32 ());
    }

    json_t* ConductorModule::dataToJson () {
        auto rootJ = ConductorCore::dataToJson ();

        json_object_set_new_int (rootJ, "currentPattern", currentPattern);
        json_object_set_new_int (rootJ, "queuedPattern", queuedPattern);

        json_object_set_new_bool (rootJ, "resetQueuedOn", resetQueuedOn);
        json_object_set_new_bool (rootJ, "resetPatternOn", resetPatternOn);
        json_object_set_new_bool (rootJ, "resetIgnoreFirstClock", resetIgnoreFirstClock);

        return rootJ;
    }

    void ConductorModule::dataFromJson (json_t* rootJ) {
        ConductorCore::dataFromJson (rootJ);

        json_object_try_get_int (rootJ, "currentPattern", currentPattern);
        json_object_try_get_int (rootJ, "queuedPattern", queuedPattern);

        json_object_try_get_bool (rootJ, "resetQueuedOn", resetQueuedOn);
        json_object_try_get_bool (rootJ, "resetPatternOn", resetPatternOn);
        json_object_try_get_bool (rootJ, "resetIgnoreFirstClock", resetIgnoreFirstClock);

        calculatePatternInfo ();
    }

    void ConductorModule::onReset (const ResetEvent& e) {
        ConductorCore::onReset (e);

        queuedPattern = QueueCleared;
        resetQueuedOn = false;
        resetPatternOn = false;
        resetIgnoreFirstClock = true;
    }

    void ConductorModule::changePattern (int newPattern) {
        assert (newPattern >= 0 && newPattern < curPatternCount);
        if (newPattern < 0 || newPattern > curPatternCount)
            return;

        currentPattern = newPattern;
        dataUpdated = true;

        calculatePatternInfo ();
    }

    void ConductorModule::queuePattern (int newPattern) {
        assert (newPattern >= 0 && newPattern < curPatternCount);
        if (newPattern < 0 || newPattern > curPatternCount)
            return;

        queuedPattern = newPattern;
        dataUpdated = true;
    }

    void ConductorModule::dequeuePattern () {
        queuedPattern = QueueCleared;
        dataUpdated = true;
    }

    void ConductorModule::handleReset () {
        resetPulse.trigger ();
        resetLightPulse.trigger (Constants::LightPulseMS);
        clockHandler.resetPulse ();

        if (curResetPattern >= 0)
            changePattern (curResetPattern);

        if (resetQueuedOn)
            dequeuePattern ();
    }

    void ConductorModule::handleAdvance () {
        if (queuedPattern != QueueCleared) {
            dataUpdated = true;

            changePattern (queuedPattern);
            dequeuePattern ();

            resetPulse.trigger ();
        }

        advanceLightPulse.trigger (Constants::LightPulseMS);
    }

    void ConductorModule::calculatePatternInfo () {
        auto sequencerMax = static_cast<int> (getParam (PARAM_SEQ_MAX_PATTERNS));
        auto newPatternCount = patternFloatToInt (Math::rescale1 (getParam (PARAM_PATTERN_COUNT), 2, sequencerMax));
        auto newPatternOffset = getParam (PARAM_PATTERN_OFFSET_VOLTAGE);
        auto newMaxCV = getParam (PARAM_SEQ_MAX_CV);

        auto patternCountChanged = newPatternCount != curPatternCount;
        auto seqPatternCountChanged = sequencerMax != curSequencerMax;
        auto patternOffsetChanged = newPatternOffset != curPatternOffset;
        auto maxCVChanged = newMaxCV != curMaxCV;
        dataUpdated |= patternCountChanged | seqPatternCountChanged | patternOffsetChanged | maxCVChanged;

        curPatternCount = newPatternCount;
        curSequencerMax = sequencerMax;
        curPatternOffset = newPatternOffset;
        curMaxCV = newMaxCV;

        if (resetPatternOn)
            curResetPattern = patternFloatToInt (Math::rescale1 (getParam (PARAM_RESET_PATTERN), 0, curPatternCount - 1));
        else
            curResetPattern = -1;

        if (currentPattern >= curPatternCount) {
            currentPattern = (curResetPattern > -1) ? curResetPattern : 0;
            dataUpdated = true;
        }
        if (queuedPattern >= curPatternCount)
            dequeuePattern ();
    }

    void ConductorModule::calculatePatternCV () {
        auto seqCVStep = curMaxCV / curSequencerMax;

        auto patternIndex = std::clamp (currentPattern, 0, std::min (curPatternCount, curSequencerMax) - 1);
        curPatternCV = patternIndex * seqCVStep + curPatternOffset * seqCVStep;
    }

    void ConductorModule::prepareDataUpdatedEvent (ConductorDataUpdatedEvent& e) {
        e.patternCount = curPatternCount;
        e.currentPattern = currentPattern;
        e.queuedPattern = queuedPattern;
    }

    void ConductorModule::process (const ProcessArgs& args) {
        ConductorCore::processCore ();

        using Constants::TriggerThreshLow;
        using Constants::TriggerThreshHigh;

        clockHandler.setParams (resetIgnoreFirstClock, static_cast<uint32_t> (getParam (PARAM_CLOCK_DELAY)));
        setOutput (OUTPUT_CLOCK, boolToGate (clockHandler.processClock (args, getInput (INPUT_CLOCK))));

        calculatePatternInfo ();

        // Process triggers and buttons.
        if (resetPatternToggleTrigger.process (getParam (PARAM_RESET_PATTERN_BUTTON)))
            resetPatternOn = !resetPatternOn;

        if (resetButtonTrigger.process (getParam (PARAM_MANUAL_RESET_BUTTON)) |
            resetTrigger.process (getInput (INPUT_RESET), TriggerThreshLow, TriggerThreshHigh))
            handleReset ();

        if (advanceButtonTrigger.process (getParam (PARAM_MANUAL_ADVANCE_BUTTON)) |
            advanceTrigger.process (getInput (INPUT_ADVANCE), TriggerThreshLow, TriggerThreshHigh))
            handleAdvance ();

        if (patternSetButtonTrigger.process (getParam (PARAM_MANUAL_SET_BUTTON))) {
            auto patternIndexNorm = getParam (PARAM_MANUAL_SET);
            auto patternIndex = patternFloatToInt (Math::rescale1 (patternIndexNorm, 0, curPatternCount - 1));
            changePattern (patternIndex);
            manualSetLightPulse.trigger (Constants::LightPulseMS);
        }

        // Process pulses.
        resetPulse.process (args.sampleTime);
        setOutput (OUTPUT_RESET, boolToGate (resetPulse.isHigh ()));

        // Output the pattern CV.
        setOutput (OUTPUT_PATTERN, curPatternCV);

        // Calculate pattern CV and emit data updates.
        if (dataUpdated) {
            dataUpdated = false;

            calculatePatternCV ();
            emitOnDataUpdated ();
        }

        // Process lights.
        if (clockLights.process ()) {
            auto lightTime = args.sampleTime * clockLights.division;

            advanceLightPulse.process (lightTime);
            resetLightPulse.process (lightTime);
            manualSetLightPulse.process (lightTime);

            setLightSmooth (LIGHT_ADVANCE_BUTTON, boolToLight (advanceLightPulse.isHigh ()), lightTime);
            setLightSmooth (LIGHT_RESET_BUTTON, boolToLight (resetLightPulse.isHigh ()), lightTime);
            setLightSmooth (LIGHT_RESET_PATTERN_BUTTON, boolToLight (resetPatternOn), lightTime);
            setLightSmooth (LIGHT_MANUAL_SET_BUTTON, boolToLight (manualSetLightPulse.isHigh ()), lightTime);
        }
    }
}