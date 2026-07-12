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

#include "Warp.hpp"

#include "../JsonUtils.hpp"
#include "../Math.hpp"

namespace OuroborosModules {
    rack::plugin::Model* modelWarp = createModel<Modules::Warp::WarpWidget> ("PhaseDistortion");
}

namespace OuroborosModules::Modules::Warp {
    WarpModule::WarpModule () {
        config (PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);

        // Configure parameters.
        configParam (PARAM_OVERSAMPLE, 1.f, MaxOversample, DefaultOversampleRate, "Oversample", "x", 0, 1);

        configParam (PARAM_AMOUNT, 0.f, 1.f, 0.f, "Distortion amount", "%", 0, 100);
        configParam (PARAM_BIAS, -MaxBias, MaxBias, 0.f, "Bias", "° degrees", 0, 180.f / MaxBias);

        configParam (PARAM_AMOUNT_CV_ATTEN, -1.f, 1.f, 0.f, "Distortion amount CV attenuverter", "%", 0, 100);
        configParam (PARAM_BIAS_CV_ATTEN, -1.f, 1.f, 0.f, "Bias CV attenuverter", "%", 0, 100);

        // Configure inputs.
        configInput (INPUT_SIGNAL, "Signal");
        configInput (INPUT_MODULATOR, "Modulator");

        configInput (INPUT_AMOUNT_CV, "Distortion amount CV");
        configInput (INPUT_BIAS_CV, "Bias CV");

        // Configure outputs.
        configOutput (OUTPUT_SIGNAL, "Signal");

        // Configure bypasses.
        configBypass (INPUT_SIGNAL, OUTPUT_SIGNAL);

        // Initialize the module.
        curSampleRate = 0;
        oversampleRate = 0;

        clockOversample = DSP::ClockDivider (7, rack::random::u32 ());

        setOversampleRate (DefaultOversampleRate);
        updateSampleRate (48000);
    }

    void WarpModule::onSampleRateChange (const SampleRateChangeEvent& e) {
        ModuleBase::onSampleRateChange (e);

        updateSampleRate (static_cast<uint32_t> (e.sampleRate));
    }

    void WarpModule::process (const ProcessArgs& args) {
        using Math::fpClean;
        using rack::simd::float_4;

        // Check for oversample updates.
        if (clockOversample.process ()) {
            const auto newOversampleRate = static_cast<int> (getParam (PARAM_OVERSAMPLE));
            setOversampleRate (newOversampleRate);
        }

        // Don't waste CPU if there's no input signal or output connected.
        if (!isInputConnected (INPUT_SIGNAL) || !isOutputConnected (OUTPUT_SIGNAL)) {
            setOutputChannels (OUTPUT_SIGNAL, 1);
            setOutput (OUTPUT_SIGNAL, 0);

            return;
        }

        // Get the channel count and set the output's.
        auto channelCount = std::min (getInputChannels (INPUT_SIGNAL), Constants::MaxPolyphony);
        setOutputChannels (OUTPUT_SIGNAL, channelCount);

        // Calculate. the bank count.
        int bankCount = channelCount / SIMDBankSize;
        if (bankCount * SIMDBankSize < channelCount)
            bankCount++;

        // Get the parameters.
        auto amountKnob = float_4 (getParam (PARAM_AMOUNT));
        auto biasKnob = float_4 (getParam (PARAM_BIAS));
        auto amountCVAtten = float_4 (getParam (PARAM_AMOUNT_CV_ATTEN));
        auto biasCVAtten = float_4 (getParam (PARAM_AMOUNT_CV_ATTEN));

        for (int bank = 0, channel = 0; channel < channelCount; bank++, channel += SIMDBankSize) {
            // Get the CV and apply it to the parameters.
            auto amount = fpClean (getInputNormalPolySimd<float_4> (INPUT_AMOUNT_CV, 0.f, channel) / 10.f * amountCVAtten);
            auto bias = fpClean (getInputNormalPolySimd<float_4> (INPUT_BIAS_CV, 0.f, channel) * biasCVAtten);

            amount = rack::simd::clamp (amountKnob + amount, 0.f, 1.f);
            bias = rack::simd::clamp ((biasKnob + bias) / MaxBias, -1.f, 1.f) * M_PI;

            // Get the signals.
            auto signal = rack::simd::clamp (fpClean (getInputPolySimd<float_4> (INPUT_SIGNAL, channel)), -100.f, 100.f);

            auto modulator = getInputNormalPolySimd<float_4> (INPUT_MODULATOR, signal, channel);
            modulator = rack::simd::clamp (fpClean (modulator), -100.f, 100.f);
            modulator = bias + (modulator * amount / MaxBias) * M_PI * 4.f;

            // Perform the hilbert transform.
            float_4 signalRe, signalIm;
            hilbertTransformSignal [bank].step (signal, signalRe, signalIm);
            auto modulatorRe = hilbertTransformModulator [bank].stepReal (modulator);

            // Upsample.
            float_4 signalBufferRe [MaxOversample];
            float_4 signalBufferIm [MaxOversample];
            float_4 modulatorBuffer [MaxOversample];

            signalReUpsampler [bank].process (signalBufferRe, signalRe);
            signalImUpsampler [bank].process (signalBufferIm, signalIm);
            modulatorUpsampler [bank].process (modulatorBuffer, modulatorRe);

            // Process the audio.
            for (uint32_t i = 0; i < oversampleRate; i += 4) {
                // Fetch the signal and modulator.
                auto phi = modulatorBuffer [i];

                // Rotate the real part of the signal.
                auto signal = signalBufferRe [i] * rack::simd::cos (phi) - signalBufferIm [i] * rack::simd::sin (phi);
                signalBufferRe [i] = fpClean (signal);
            }

            // Downsample, perform DC blocking and output.
            auto output = downsamplerFilter [bank].process (signalBufferRe);
            output = dcBlocker [bank].process (output);
            setOutputSimd (OUTPUT_SIGNAL, output, channel);
        }
    }

    void WarpModule::setOversampleRate (uint32_t newOversampleRate) {
        assert (newOversampleRate > 0);
        assert (newOversampleRate <= MaxOversample);

        if (newOversampleRate == oversampleRate)
            return;

        oversampleRate = newOversampleRate;

        for (int bank = 0; bank < SIMDBankCount; bank++) {
            signalReUpsampler [bank].setParams (newOversampleRate);
            signalImUpsampler [bank].setParams (newOversampleRate);
            modulatorUpsampler [bank].setParams (newOversampleRate);
            downsamplerFilter [bank].setParams (newOversampleRate);
        }
    }

    void WarpModule::updateSampleRate (uint32_t newSampleRate) {
        if (newSampleRate == curSampleRate)
            return;

        curSampleRate = newSampleRate;
        for (int bank = 0; bank < SIMDBankCount; bank++) {
            hilbertTransformSignal [bank].setSampleRate (newSampleRate);
            hilbertTransformModulator [bank].setSampleRate (newSampleRate);

            dcBlocker [bank].setCutoffFreq (Constants::DefaultDCBlockerCutoff, newSampleRate);
        }
    }
}