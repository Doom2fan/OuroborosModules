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
        config (NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);

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
        // Check for oversample updates.
        if (clockOversample.process ()) {
            const auto newOversampleRate = static_cast<int> (params [PARAM_OVERSAMPLE].getValue ());
            setOversampleRate (newOversampleRate);
        }

        // Don't waste CPU if there's no input signal or output connected.
        if (!inputs [INPUT_SIGNAL].isConnected () || !outputs [OUTPUT_SIGNAL].isConnected ()) {
            outputs [OUTPUT_SIGNAL].setChannels (1);
            outputs [OUTPUT_SIGNAL].setVoltage (0);

            return;
        }

        auto channelCount = std::min (inputs [INPUT_SIGNAL].getChannels (), Constants::MaxPolyphony);
        outputs [OUTPUT_SIGNAL].setChannels (channelCount);
        for (int channel = 0; channel < channelCount; channel++)
            processChannel (channel);
    }

    void WarpModule::processChannel (int channel) {
        using rack::simd::float_4;

        auto amount = params [PARAM_AMOUNT].getValue ()
                    + inputs [INPUT_AMOUNT_CV].getNormalPolyVoltage (0.f, channel) / 10.f
                    * params [PARAM_AMOUNT_CV_ATTEN].getValue ();
        auto bias = params [PARAM_BIAS].getValue ()
                  + inputs [INPUT_BIAS_CV].getNormalPolyVoltage (0.f, channel)
                  * params [PARAM_BIAS_CV_ATTEN].getValue ();

        amount = std::clamp (amount, 0.f, 1.f);
        bias = std::clamp (bias / MaxBias, -1.f, 1.f) * M_PI;

        // Get signals.
        auto signal = inputs [INPUT_SIGNAL].getPolyVoltage (channel);
        auto modulator = inputs [INPUT_MODULATOR].getNormalPolyVoltage (signal, channel) * amount / MaxBias;
        modulator = bias + modulator * M_PI * 4.f;

        // Oversample.
        float signalBuffer [MaxOversample];
        float modulatorBuffer [MaxOversample];
        float signalBufferIm [MaxOversample];

        signalUpsampler [channel].process (signalBuffer, signal);
        upsamplerFilter [channel].process (modulatorBuffer, modulator);

        // Perform the hilbert transform.
        for (uint32_t i = 0; i < oversampleRate; i++) {
            std::tie (signalBuffer [i], signalBufferIm [i]) = hilbertTransformSignal [channel].stepPair (signalBuffer [i]);
            modulatorBuffer [i] = hilbertTransformModulator [channel].stepPair (modulatorBuffer [i]).first;
        }

        // Process the audio.
        for (uint32_t i = 0; i < oversampleRate; i += 4) {
            // Fetch the signal and modulator.
            auto phi = float_4::load (modulatorBuffer + i);
            std::complex<float_4> c (float_4::load (signalBuffer + i), float_4::load (signalBufferIm + i));

            // Rotate the real part of the signal.
            auto signal = c.real () * rack::simd::cos (phi) - c.imag () * rack::simd::sin (phi);

            signal.store (signalBuffer + i);
        }

        auto output = downsamplerFilter [channel].process (signalBuffer);
        output = dcBlocker [channel].process (output);
        outputs [OUTPUT_SIGNAL].setVoltage (output, channel);
    }

    void WarpModule::setOversampleRate (uint32_t newOversampleRate) {
        assert (newOversampleRate > 0);
        assert (newOversampleRate <= MaxOversample);

        if (newOversampleRate == oversampleRate)
            return;

        oversampleRate = newOversampleRate;

        for (int channel = 0; channel < Constants::MaxPolyphony; channel++) {
            signalUpsampler [channel].setParams (newOversampleRate);
            upsamplerFilter [channel].setParams (newOversampleRate);
            downsamplerFilter [channel].setParams (newOversampleRate);
        }
    }

    void WarpModule::updateSampleRate (uint32_t newSampleRate) {
        if (newSampleRate == curSampleRate)
            return;

        curSampleRate = newSampleRate;
        for (int channel = 0; channel < Constants::MaxPolyphony; channel++) {
            hilbertTransformSignal [channel].setSampleRate (newSampleRate * oversampleRate);
            hilbertTransformModulator [channel].setSampleRate (newSampleRate * oversampleRate);

            dcBlocker [channel].setCutoffFreq (Constants::DefaultDCBlockerCutoff, newSampleRate);
        }
    }
}