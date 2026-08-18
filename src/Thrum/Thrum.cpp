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

#include "Thrum.hpp"

#include "../JsonUtils.hpp"

namespace OuroborosModules {
    rack::plugin::Model* modelThrum = createModel<Modules::Thrum::ThrumWidget> ("Thrum");
}

namespace OuroborosModules::Modules::Thrum {
    SnareFilterType snareFilterFromParam (float param) {
        return param < 0.5 ? SnareFilterType::Bandpass : SnareFilterType::Lowpass;
    }

    float vOctToHz (float vOct) {
        return rack::dsp::exp2_taylor5 (vOct) * rack::dsp::FREQ_C4;
    }

    ThrumModule::ThrumModule () {
        auto inf = std::numeric_limits<float>::infinity ();

        config (NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);

        // Configure parameters
        configParamVOctFromHz (PARAM_FREQ, MinFreqHz, MaxFreqHz, DefaultFreqHz, "Frequency");

        configParamDecibels (PARAM_EXCITE_SINE_LEVEL, -inf, 0, 0, "Exciter sine level");
        configParamDecibels (PARAM_EXCITE_NOISE_LEVEL, -inf, 0, 0, "Exciter noise level");

        configParamDecibels (PARAM_EXCITE_NOISE_BLEED_LEVEL, -inf, 0, 0, "Exciter noise bleed level");

        configParam (PARAM_FEEDBACK, -1, 1, .5f, "Feedback", "%", 0, 100);
        configParamVOctFromHz (PARAM_DAMPING, MinFreqHz, MaxFreqHz, 5000.f, "Damping");

        configParamVOctFromHz (PARAM_ALLPASS_FREQ, MinAPFreqHz, MaxAPFreqHz, DefaultAPFreqHz, "Allpass frequency");
        configParam (PARAM_ALLPASS_GAIN, -1, 1, .3f, "Allpass gain", "%", 0, 100);

        configParam (PARAM_PITCHBEND_DECAY, 1 / 1000.f, 300 / 1000.f, 80 / 1000.f, "Pitch bend decay", " ms", 0, 1000);
        configParam (PARAM_PITCHBEND_DEPTH, 0, 1, 1, "Pitch bend depth", "%", 0, 100);

        configParamDecibels (PARAM_SNARE_LEVEL, -inf, 0, -18, "Snare noise level");
        configParamVOctFromHz (PARAM_SNARE_FILTER_FREQ,
            MinSnareFreqHz, MaxSnareFreqHz, DefaultSnareFreqHz, "Snare noise filter frequency");
        configSwitch (PARAM_SNARE_FILTER_TYPE, 0.f, 1.f, 0.f, "Snare noise filter type", { "Bandpass", "Lowpass", });
        configParam (PARAM_SNARE_FALL_TIME, 5.f / 1000.f, 500.f / 1000.f, 250.f / 1000.f, "Snare envelope follower fall time", " ms", 0, 1000);

        configParam (PARAM_VELOCITY, 0, 1, 1, "Velocity", "%", 0, 100);

        configSwitch (PARAM_RESET_ON_HIT, 0.f, 1.f, 1.f, "Reset on hit", { "Off", "On", });

        // Configure inputs
        configInput (INPUT_TRIGGER, "Trigger");
        configInput (INPUT_VELOCITY, "Velocity");

        // Configure outputs
        configOutput (OUTPUT_SIGNAL, "Audio");

        // Schmitt triggers
        for (int channel = 0; channel < Constants::MaxPolyphony; channel++)
            hitTrigger [channel] = rack::dsp::SchmittTrigger ();
    }

    void ThrumModule::onSampleRateChange (const SampleRateChangeEvent& e) {
        ModuleBase::onSampleRateChange (e);

        // Clock dividers
        clockParams = DSP::ClockDivider (static_cast<uint32_t> (e.sampleRate / (48000.f / 32)), rack::random::u32 ());

        // Drum cores
        for (int channel = 0; channel < Constants::MaxPolyphony; channel++)
            drumCores [channel].onSampleRateChange (e.sampleRate);
    }

    void ThrumModule::process (const ProcessArgs& args) {
        using Constants::TriggerThreshHigh;
        using Constants::TriggerThreshLow;
        using Math::fpClean;

        auto channelCount = std::max (1, getInputChannels (INPUT_TRIGGER));

        auto baseVelocity = getParam (PARAM_VELOCITY);
        auto resetOnHit = getParam (PARAM_RESET_ON_HIT) > .5f;
        for (int channel = 0; channel < channelCount; channel++) {
            auto& drumCore = drumCores [channel];
            if (hitTrigger [channel].process (getInput (INPUT_TRIGGER, channel), TriggerThreshLow, TriggerThreshHigh)) {
                auto velocity = std::clamp (baseVelocity + fpClean (getInputPoly (INPUT_VELOCITY, channel)) / 10.f, 0.f, 1.f);

                drumCore.setFrequency (vOctToHz (getParam (PARAM_FREQ)));

                drumCore.setExciterParams (getParam (PARAM_EXCITE_SINE_LEVEL), getParam (PARAM_EXCITE_NOISE_LEVEL));
                drumCore.setExciterBleedParams (getParam (PARAM_EXCITE_NOISE_BLEED_LEVEL));

                drumCore.setFeedback (getParam (PARAM_FEEDBACK), vOctToHz (getParam (PARAM_DAMPING)));
                drumCore.setAllpassParams (vOctToHz (getParam (PARAM_ALLPASS_FREQ)), getParam (PARAM_ALLPASS_GAIN));

                drumCore.setPitchbendParams (getParam (PARAM_PITCHBEND_DECAY), getParam (PARAM_PITCHBEND_DEPTH));
                drumCore.setSnareParams (
                    getParam (PARAM_SNARE_LEVEL),
                    vOctToHz (getParam (PARAM_SNARE_FILTER_FREQ)),
                    snareFilterFromParam (getParam (PARAM_SNARE_FILTER_TYPE)),
                    getParam (PARAM_SNARE_FALL_TIME)
                );

                drumCore.hit (velocity, resetOnHit);
            }

            auto signal = drumCore.process (args) * 5.f;
            setOutput (OUTPUT_SIGNAL, signal, channel);
        }
    }
}