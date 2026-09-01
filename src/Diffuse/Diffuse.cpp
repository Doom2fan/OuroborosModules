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

#include "Diffuse.hpp"

namespace OuroborosModules {
    rack::plugin::Model* modelDiffuse = createModel<Modules::Diffuse::DiffuseWidget> ("Diffuse");
}

namespace OuroborosModules::Modules::Diffuse {
    static constexpr float MinFreqHz = 32.f;
    static constexpr float MaxFreqHz = 20000.f;

    DiffuseModule::DiffuseModule () {
        config (NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);

        // Configure parameters.
        configParamVOctFromHz (PARAM_FREQ, MinFreqHz, MaxFreqHz, rack::dsp::FREQ_C4, "Frequency");
        configParam (PARAM_GAIN, -1.f, 1.f, 0.f, "Gain", "%", 0, 100);
        configParam (PARAM_MIX, 0.f, 1.f, .5f, "Mix", "%", 0, 100);

        configParamAttenuverter (PARAM_FREQ_CV_ATTEN, "Linear FM CV attenuverter");
        configParamAttenuverter (PARAM_GAIN_CV_ATTEN, "Gain CV attenuverter");
        configParamAttenuverter (PARAM_MIX_CV_ATTEN, "Dry/Wet mix CV attenuverter");

        // Configure inputs.
        configInput (INPUT_SIGNAL, "Signal");
        configInput (INPUT_VOCT, "1V/Oct");

        configInput (INPUT_FREQ_CV, "Linear FM CV");
        configInput (INPUT_GAIN_CV, "Gain CV");
        configInput (INPUT_MIX_CV, "Dry/Wet mix CV");

        // Configure outputs.
        configOutput (OUTPUT_WET, "Wet");
        configOutput (OUTPUT_MIX, "Mix");

        // Configure bypasses.
        configBypass (INPUT_SIGNAL, OUTPUT_WET);
        configBypass (INPUT_SIGNAL, OUTPUT_MIX);
    }

    static float calculateDelayFreq (float vOct) {
        return rack::dsp::FREQ_C4 * rack::dsp::exp2_taylor5 (vOct);
    }

    void DiffuseModule::onSampleRateChange (const SampleRateChangeEvent& e) {
        ModuleBase::onSampleRateChange (e);

        maxDelayFreq = std::min (MaxFreqHz, e.sampleRate / 2.f);
        auto maxDelaySamples = static_cast<int32_t> (std::ceil (e.sampleRate / MinFreqHz + 5.f));

        for (int channel = 0; channel < Constants::MaxPolyphony; channel++) {
            allpass [channel].setMaxDelay (maxDelaySamples);
            allpass [channel].resetFull ();
        }
    }

    void DiffuseModule::process (const ProcessArgs& args) {
        using Math::fpClean;

        if (!isOutputConnected (OUTPUT_WET) && !isOutputConnected (OUTPUT_MIX))
            return;

        // Pre-fetch the params.
        auto freqKnob = getParam (PARAM_FREQ);
        auto gainKnob = getParam (PARAM_GAIN);
        auto mixKnob = getParam (PARAM_MIX);

        auto freqCVKnob = getParam (PARAM_FREQ_CV_ATTEN);
        auto gainCVKnob = getParam (PARAM_GAIN_CV_ATTEN);
        auto mixCVKnob = getParam (PARAM_MIX_CV_ATTEN);

        // Calculate and set the polyphony count.
        auto channelCount = std::max (1, getInputChannels (INPUT_SIGNAL));
        setOutputChannels (OUTPUT_WET, channelCount);
        setOutputChannels (OUTPUT_MIX, channelCount);

        // Process.
        for (int channel = 0; channel < channelCount; channel++) {
            // Calculate "pitch" from knob + V/Oct.
            auto vOct = freqKnob + fpClean (getInputPoly (INPUT_VOCT, channel));
            auto delayFreq = std::clamp (calculateDelayFreq (vOct), MinFreqHz, maxDelayFreq);

            // Apply FM.
            auto fm = fpClean (getInputPoly (INPUT_FREQ_CV, channel) / 5.f * freqCVKnob * 5000.f);
            delayFreq = std::clamp (delayFreq + fm, MinFreqHz, maxDelayFreq);

            // Process.
            allpass [channel].setDelayTime (args.sampleRate / delayFreq);

            auto gain = gainKnob + fpClean (getInputPoly (INPUT_GAIN_CV, channel) / 5.f * gainCVKnob);
            allpass [channel].setGain (std::clamp (gain, -1.f, 1.f));

            auto input = fpClean (getInputPoly (INPUT_SIGNAL, channel));
            auto wet = allpass [channel].process (input);

            // Dry/wet mix.
            auto mixRatio = std::clamp (mixKnob + fpClean (getInputPoly (INPUT_MIX_CV, channel) / 10.f) * mixCVKnob, 0.f, 1.f);
            auto mix = input * (1.f - mixRatio) + wet * mixRatio;

            setOutput (OUTPUT_WET, wet, channel);
            setOutput (OUTPUT_MIX, mix, channel);
        }
    }

    void DiffuseModule::onUnBypass (const UnBypassEvent& e) {
        ModuleBase::onUnBypass (e);

        for (int channel = 0; channel < Constants::MaxPolyphony; channel++)
            allpass [channel].resetFull ();
    }
}