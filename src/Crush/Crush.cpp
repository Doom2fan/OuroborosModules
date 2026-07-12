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

#include "Crush.hpp"

#include "../Math.hpp"
#include "../JsonUtils.hpp"

namespace OuroborosModules {
    rack::plugin::Model* modelCrush = createModel<Modules::Crush::CrushWidget> ("Crush");
}

namespace OuroborosModules::Modules::Crush {
    CrushModule::CrushModule () {
        config (NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);

        // Configure parameters.
        configParam (PARAM_TARGET_LEVEL, rack::dsp::dbToAmplitude (-40.f), 2.f, rack::dsp::dbToAmplitude (-3.f), "Target level", " dB", -10, 20);
        configParam (PARAM_AMOUNT, 0.f, 1.f, 1.f, "Depth", "%", 0, 100);

        configParam (PARAM_FILTER_TIME, 5.f / 1000.f, 500.f / 1000.f, 250.f / 1000.f, "Fall time", " ms", 0, 1000);

        configParam (PARAM_AMOUNT_CV_ATTEN, -1.f, 1.f, 0.f, "Depth CV attenuverter", "%", 0, 100);

        // Configure inputs.
        configInput (INPUT_SIGNAL, "Signal");

        configInput (INPUT_TARGET_LEVEL, "Target level");
        configInput (INPUT_AMOUNT_CV, "Depth CV");

        // Configure outputs.
        configOutput (OUTPUT_SIGNAL, "Signal");
        configOutput (OUTPUT_ENVELOPE, "Envelope");

        // Configure bypasses.
        configBypass (INPUT_SIGNAL, OUTPUT_SIGNAL);

        // Initialize the module.
        clockParams = DSP::ClockDivider (32, rack::random::u32 ());

        setPeakFilter ();
    }

    void CrushModule::dataFromJson (json_t* rootJ) {
        ModuleBase::dataFromJson (rootJ);

        setPeakFilter ();
    }

    void CrushModule::setPeakFilter () {
        auto filterTime = rack::simd::float_4 (getParam (PARAM_FILTER_TIME) / 10.f);
        for (int bank = 0; bank < SIMDBankCount; bank++)
            peakFilter [bank].setTau (filterTime);
    }

    void CrushModule::process (const ProcessArgs& args) {
        using rack::simd::float_4;

        // Get the mono params.
        auto minimumLevelRcp = float_4 (1.f / (rack::dsp::dbToAmplitude (-96.f) * 5.f));
        auto targetLevelKnob = float_4 (getParam (PARAM_TARGET_LEVEL) * 5.f);
        auto amountKnob = float_4 (getParam (PARAM_AMOUNT));

        // Calculate polyphony and SIMD counts, and set output polyphony counts.
        const int channelCount = std::max (1, getInputChannels (INPUT_SIGNAL));
        int bankCount = channelCount / SIMDBankSize;
        if (bankCount * SIMDBankSize < channelCount)
            bankCount++;

        setOutputChannels (OUTPUT_SIGNAL, channelCount);
        setOutputChannels (OUTPUT_ENVELOPE, channelCount);

        // Update params.
        if (clockParams.process ())
            setPeakFilter ();

        // Process the audio.
        for (int bank = 0, channel = 0; bank < bankCount; bank++, channel += SIMDBankSize) {
            // Get the per-channel params.
            auto targetLevel = Math::fpClean (rack::simd::fmax (
                getInputNormalPolySimd<float_4> (INPUT_TARGET_LEVEL, targetLevelKnob, channel),
                1.f
            ));
            auto amount = rack::simd::clamp (amountKnob + Math::fpClean (
                          getInputPolySimd<float_4> (INPUT_AMOUNT_CV, channel) / 10.f *
                          getParam (PARAM_AMOUNT_CV_ATTEN)), float_4::zero (), 1.f);

            // Get and clean the input signal.
            auto inputSignal = Math::fpClean (getInputPolySimd<float_4> (INPUT_SIGNAL, channel));
            inputSignal = rack::simd::clamp (inputSignal, -100, 100);

            // Calculate amplitude using the peak filter.
            auto amplitude = Math::fpClean (peakFilter [bank].process (args.sampleTime, rack::simd::abs (inputSignal)));
            setOutputSimd (OUTPUT_ENVELOPE, amplitude, channel);

            // Calculate gain while ensuring amplitude is above a certain level.
            auto gain = 1.f + (rack::simd::fmin (1.f / amplitude, minimumLevelRcp) * targetLevel - 1.f) * amount;

            // Calculate the output signal.
            auto output = inputSignal * gain;
            output = dcBlocker [bank].process (output);

            setOutputSimd (OUTPUT_SIGNAL, output, channel);
        }
    }

    void CrushModule::onSampleRateChange (const SampleRateChangeEvent& e) {
        ModuleBase::onSampleRateChange (e);

        auto newSampleRate = static_cast<uint32_t> (e.sampleRate);
        if (newSampleRate == curSampleRate)
            return;

        curSampleRate = newSampleRate;
        for (int bank = 0; bank < SIMDBankCount; bank++) {
            dcBlocker [bank].setCutoffFreq (Constants::DefaultDCBlockerCutoff, newSampleRate);
        }
    }
}