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

#include "Pulsar.hpp"

#include "../JsonUtils.hpp"
#include "../Math.hpp"
#include "../ModuleHelpers.hpp"

namespace OuroborosModules {
    rack::plugin::Model* modelPulsar = createModel<Modules::Pulsar::PulsarWidget> ("Pulsar");
}

namespace OuroborosModules::Modules::Pulsar {
    /*
     * Burst count helpers
     */
    uint32_t burstToInt (float v) {
        return static_cast<uint32_t> (std::round (v));
    }

    struct BurstCountQuantity : rack::engine::ParamQuantity {
        float getDisplayValue () override {
            auto module = reinterpret_cast<PulsarModule*> (this->module);

            if (module != nullptr && module->maskingMode == PulsarMaskingMode::Burst)
                return burstToInt (ParamQuantity::getDisplayValue ());

            return ParamQuantity::getDisplayValue ();
        }
    };

    /*
     * Formant helpers
     */
    static const float FormantMinVOct = -4;
    static const float FormantMaxVOct = 6;
    template<typename T>
    T decoupledFormantFromParam (T paramVal, T vOct) {
        vOct += Math::rescale1 (paramVal, T (FormantMinVOct), T (FormantMaxVOct));
        return rack::dsp::FREQ_C4 * rack::dsp::exp2_taylor5 (vOct);
    }

    template<typename T>
    T decoupledFormantToParam (T freqVal) {
        auto vOct = rack::simd::log2 (freqVal / rack::dsp::FREQ_C4);
        auto normalized = rack::math::rescale (vOct, T (FormantMinVOct), T (FormantMaxVOct), 0, 1);
        return rack::simd::clamp (normalized, T (FormantMinVOct), T (FormantMaxVOct));
    }

    struct FormantQuantity : rack::engine::ParamQuantity {
        float getDisplayValue () override {
            auto module = reinterpret_cast<PulsarModule*> (this->module);

            if (module != nullptr && module->formantDecoupled)
                return decoupledFormantFromParam (ParamQuantity::getDisplayValue (), 0.f);

            return ParamQuantity::getDisplayValue ();
        }

        void setDisplayValue (float displayValue) override {
            auto module = reinterpret_cast<PulsarModule*> (this->module);

            if (module != nullptr && module->formantDecoupled) {
                ParamQuantity::setDisplayValue (decoupledFormantToParam (displayValue));
                return;
            }

            ParamQuantity::setDisplayValue (displayValue);
        }

        float getDefaultValue () override {
            auto module = reinterpret_cast<PulsarModule*> (this->module);

            if (module != nullptr && module->formantDecoupled) {
                return decoupledFormantToParam (rack::dsp::FREQ_C4);
            } else
                return defaultValue;
        }
    };

    /*
     * Masking mode helpers
     */
    PulsarMaskingMode maskingModeFromParam (float paramVal) {
        switch (static_cast<int> (std::round (paramVal))) {
            default:
            case 0: return PulsarMaskingMode::Burst;
            case 1: return PulsarMaskingMode::Stochastic;
        }
    }

    /*
     *
     */
    PulsarFrequencyMode frequencyModeFromParam (float paramVal) {
        switch (static_cast<int> (std::round (paramVal))) {
            default:
            case 0: return PulsarFrequencyMode::Audio;
            case 1: return PulsarFrequencyMode::LFO;
            case 2: return PulsarFrequencyMode::Triggered;
        }
    }

    /*
     * PulsarModule
     */
    PulsarModule::PulsarModule () {
        config (NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);

        /* Configure parameters */
        // Frequency
        configParamVOct (PARAM_FREQUENCY, -4, 6, 0, "Frequency");
        configParam<FormantQuantity> (PARAM_FORMANT, 0.f, 1.f, 0.f, "Formant", "%", 0, 100);
        configParam (PARAM_CLUSTER, 1.f, 6.f, 1.f, "Cluster", "", 0, 1);

        // Wave shape
        configParam (PARAM_WAVEINDEX, 0.f, WavesCount - 1.f, 0.f, "Waveform index", "", 0, 1, 1);
        configParam (PARAM_WAVESHAPER, 0.f, 1.f, 0.f, "Waveshaper", "", 0, 100);

        // Windowing
        configParam (PARAM_WINDOWINDEX, 0.f, WindowsCount - 1.f, 0.f, "Window shape", "", 0, 1, 1);
        configParam (PARAM_WINDOWSKEW, 0.f, 1.f, .5f, "Window skew", "%", 0, 200, -100);
        configParam (PARAM_WINDOWAMOUNT, 0.f, 1.f, 0.f, "Window amount", "%", 0, 100);

        // Masking parameters
        configParam<BurstCountQuantity> (PARAM_BURSTCOUNT, 0.f, 1.f, 0.f, "", "");
        configParam<BurstCountQuantity> (PARAM_RESTCOUNT, 0.f, 1.f, 0.f, "", "");

        // Mode switches
        configSwitch (PARAM_MASKINGMODE, 0, 1, 0, "Masking mode", { "Burst/Channel", "Probability" });
        configSwitch (PARAM_DECOUPLE, 0, 1, 0, "Frequency decoupling", { "Off", "On" });

        // CV attenuverters
        configParamAttenuverter (PARAM_FORMANT_CV_ATTEN, "Formant CV attenuverter");
        configParamAttenuverter (PARAM_CLUSTER_CV_ATTEN, "Cluster CV attenuverter");

        configParamAttenuverter (PARAM_WAVEINDEX_CV_ATTEN, "Waveform index CV attenuverter");
        configParamAttenuverter (PARAM_WAVESHAPER_CV_ATTEN, "Waveshaper CV attenuverter");

        configParamAttenuverter (PARAM_WINDOWINDEX_CV_ATTEN, "Window shape CV attenuverter");
        configParamAttenuverter (PARAM_WINDOWSKEW_CV_ATTEN, "Window skew CV attenuverter");
        configParamAttenuverter (PARAM_WINDOWAMOUNT_CV_ATTEN, "Window amount CV attenuverter");

        configParamAttenuverter (PARAM_BURSTCOUNT_CV_ATTEN);
        configParamAttenuverter (PARAM_RESTCOUNT_CV_ATTEN);

        // Settings
        configParam (PARAM_OVERSAMPLE, 1.f, MaxOversample, DefaultOversampleRate, "Oversample", "x", 0, 1);
        configParam (PARAM_CHANNEL_COUNT, 0.f, Constants::MaxPolyphony, 0.f, "Channel count");
        configParam (PARAM_OVERLAP_MODE, 0.f, 1.f, 0.f, "Overlap mode");
        configParam (PARAM_EDGE_FACTOR, 0.f, 15 / 1000.f, 2 / 1000.f, "Edge factor", " ms", 0, 1000);
        configSwitch (PARAM_FREQUENCY_MODE, 0, 2, 0, "Frequency mode", { "Audio rate", "Low frequency", "Triggered" });

        // Disable randomization for relevant params
        getParamQuantity (PARAM_OVERSAMPLE)->randomizeEnabled = false;
        getParamQuantity (PARAM_CHANNEL_COUNT)->randomizeEnabled = false;
        getParamQuantity (PARAM_OVERLAP_MODE)->randomizeEnabled = false;
        getParamQuantity (PARAM_EDGE_FACTOR)->randomizeEnabled = false;

        /* Configure inputs */
        // Frequency CV
        configInput (INPUT_VOCT, "V/Oct");
        configInput (INPUT_FORMANT_CV, "Formant CV");
        configInput (INPUT_CLUSTER_CV, "Cluster CV");

        // Wave shape CV
        configInput (INPUT_WAVEINDEX_CV, "Waveform index CV");
        configInput (INPUT_WAVESHAPER_CV, "Waveshaper CV");

        // Windowing CV
        configInput (INPUT_WINDOWINDEX_CV, "Window shape CV");
        configInput (INPUT_WINDOWSKEW_CV, "Window skew CV");
        configInput (INPUT_WINDOWAMOUNT_CV, "Window amount CV");

        // Masking CV
        configInput (INPUT_BURSTCOUNT_CV);
        configInput (INPUT_RESTCOUNT_CV);
        configInput (INPUT_RESTCOUNT_CV);

        // Misc
        configInput (INPUT_SYNC, "Sync");

        /* Configure outputs */
        configOutput (OUTPUT_MAIN, "Pulsar");
        configOutput (OUTPUT_REST, "Rest");

        /* Initialize */
        curSampleRate = 48'000.f;
        curSampleTime = 1.f / curSampleRate;
    }

    json_t* PulsarModule::dataToJson () {
        auto rootJ = ModuleBase::dataToJson ();

        json_object_set_new_struct (rootJ, "pulsarEngine", engine);

        return rootJ;
    }

    void PulsarModule::dataFromJson (json_t* rootJ) {
        ModuleBase::dataFromJson (rootJ);

        json_object_try_get_struct (rootJ, "pulsarEngine", engine);

        // Initialize module
        initializeModule ();
    }

    void PulsarModule::initializeModule () {
        engine.onSampleRateChange (curSampleRate);

        setMaskingMode (maskingModeFromParam (getParam (PARAM_MASKINGMODE)), true);
        setFormantDecouple (getParam (PARAM_DECOUPLE) > .5f, true);
        setOverlapMode (getParam (PARAM_OVERLAP_MODE) > .5f, true);
        setFrequencyMode (frequencyModeFromParam (getParam (PARAM_FREQUENCY_MODE)), true);

        updateOversampleRate ();
        updateParams ();
    }

    void PulsarModule::updateOversampleRate () {
        auto factor = static_cast<uint8_t> (getParam (PARAM_OVERSAMPLE));
        factor = std::clamp (Math::nextPowerOfTwo (factor), 1_u8, MaxOversample);

        engine.setOversampling (factor, false);
    }

    void PulsarModule::setMaskingMode (PulsarMaskingMode mode, bool force) {
        if (!force && mode == maskingMode)
            return;

        auto burstCountQuantity = getParamQuantity (PARAM_BURSTCOUNT);
        auto restCountQuantity = getParamQuantity (PARAM_RESTCOUNT);
        switch (mode) {
            default: // Don't allow invalid modes
                return;

            case PulsarMaskingMode::Burst: {
                burstCountQuantity->name = "Burst count";
                burstCountQuantity->unit = "";
                burstCountQuantity->displayMultiplier = 6;
                burstCountQuantity->displayOffset = 1;

                restCountQuantity->name = "Rest count";
                restCountQuantity->unit = "";
                restCountQuantity->displayMultiplier = 7;
                restCountQuantity->displayOffset = 0;

                inputInfos [INPUT_BURSTCOUNT_CV]->name = "Burst count CV";
                inputInfos [INPUT_RESTCOUNT_CV]->name = "Rest count CV";
                getParamQuantity (PARAM_BURSTCOUNT_CV_ATTEN)->name = "Burst count CV attenuverter";
                getParamQuantity (PARAM_RESTCOUNT_CV_ATTEN)->name = "Rest count CV attenuverter";

                break;
            }

            case PulsarMaskingMode::Stochastic: {
                burstCountQuantity->name = "Rest probability";
                burstCountQuantity->unit = "%";
                burstCountQuantity->displayMultiplier = 100;
                burstCountQuantity->displayOffset = 0;

                restCountQuantity->name = "Skip probability";
                restCountQuantity->unit = "%";
                restCountQuantity->displayMultiplier = 100;
                restCountQuantity->displayOffset = 0;

                inputInfos [INPUT_BURSTCOUNT_CV]->name = "Rest probability CV";
                inputInfos [INPUT_RESTCOUNT_CV]->name = "Skip probability CV";
                getParamQuantity (PARAM_BURSTCOUNT_CV_ATTEN)->name = "Rest probability CV attenuverter";
                getParamQuantity (PARAM_RESTCOUNT_CV_ATTEN)->name = "Skip probability CV attenuverter";

                break;
            }
        }

        maskingMode = mode;
    }

    void PulsarModule::setFormantDecouple (bool decoupled, bool force) {
        if (!force && decoupled == formantDecoupled)
            return;

        formantDecoupled = decoupled;

        auto formantQuantity = getParamQuantity (PARAM_FORMANT);
        if (decoupled) {
            formantQuantity->displayMultiplier = 1.f;
            formantQuantity->unit = " Hz";
            formantQuantity->defaultValue = decoupledFormantToParam (rack::dsp::FREQ_C4);
        } else {
            formantQuantity->displayMultiplier = 100.f;
            formantQuantity->unit = "%";
            formantQuantity->defaultValue = 0.f;
        }
    }

    void PulsarModule::setOverlapMode (bool overlap, bool force) {
        if (!force && overlap == overlapMode)
            return;

        overlapMode = overlap;

        engine.setOverlapMode (overlap);
    }

    void PulsarModule::setFrequencyMode (PulsarFrequencyMode mode, bool force) {
        if (!force && mode == frequencyMode)
            return;

        switch (mode) {
            default:
            case PulsarFrequencyMode::Audio: {
                frequencyMode = PulsarFrequencyMode::Audio;
                engine.setTriggeredMode (false);
                break;
            }

            case PulsarFrequencyMode::LFO: {
                frequencyMode = PulsarFrequencyMode::LFO;
                engine.setTriggeredMode (false);
                break;
            }

            case PulsarFrequencyMode::Triggered: {
                frequencyMode = PulsarFrequencyMode::Triggered;
                engine.setTriggeredMode (true);
                break;
            }
        }
    }

    void PulsarModule::onSampleRateChange (const SampleRateChangeEvent& e) {
        ModuleBase::onSampleRateChange (e);

        curSampleRate = e.sampleRate;
        curSampleTime = e.sampleTime;

        initializeModule ();

        // Clock dividers
        clockOversample = DSP::ClockDivider (static_cast<uint32_t> (e.sampleRate / (48000.f / 128)), rack::random::u32 ());
        clockParams = DSP::ClockDivider (static_cast<uint32_t> (e.sampleRate / (48000.f / 32)), rack::random::u32 ());
    }

    void PulsarModule::onReset (const ResetEvent& e) {
        // Perform the reset twice to ensure everything gets reset properly
        for (int i = 0; i < 2; i++) {
            ModuleBase::onReset (e);
            initializeModule ();
        }
    }

    void PulsarModule::onRandomize (const RandomizeEvent& e) {
        ModuleBase::onRandomize (e);
        initializeModule ();
    }

    uint8_t PulsarModule::getChannelCount () {
        auto channelCount = static_cast<int32_t> (getParam (PARAM_CHANNEL_COUNT));

        // Manually set channel count
        if (channelCount > 0)
            return static_cast<uint8_t> (std::clamp (channelCount, 1, Constants::MaxPolyphony));

        channelCount = 1;
        channelCount = std::max (channelCount, getInputChannels (INPUT_VOCT));
        channelCount = std::max (channelCount, getInputChannels (INPUT_FORMANT_CV));

        return static_cast<uint8_t> (std::clamp (channelCount, 1, Constants::MaxPolyphony));
    }

    void PulsarModule::updateParams () {
        using Math::fpClean;

        // Set the whole-engine settings
        setMaskingMode (maskingModeFromParam (getParam (PARAM_MASKINGMODE)));
        setFormantDecouple (getParam (PARAM_DECOUPLE) > .5f);
        setOverlapMode (getParam (PARAM_OVERLAP_MODE) > .5f);
        setFrequencyMode (frequencyModeFromParam (getParam (PARAM_FREQUENCY_MODE)));

        // Fetch CV attenuverters
        auto formantKnob = AutoAttenuverter (this, PARAM_FORMANT, PARAM_FORMANT_CV_ATTEN, 10);
        auto clusterKnob = AutoAttenuverter (this, PARAM_CLUSTER, PARAM_CLUSTER_CV_ATTEN, 10);

        auto waveIndexKnob = AutoAttenuverter (this, PARAM_WAVEINDEX, PARAM_WAVEINDEX_CV_ATTEN, 10);
        auto waveshaperKnob = AutoAttenuverter (this, PARAM_WAVESHAPER, PARAM_WAVESHAPER_CV_ATTEN, 10);

        auto windowIndexKnob = AutoAttenuverter (this, PARAM_WINDOWINDEX, PARAM_WINDOWINDEX_CV_ATTEN, 10);
        auto windowSkewKnob = AutoAttenuverter (this, PARAM_WINDOWSKEW, PARAM_WINDOWSKEW_CV_ATTEN, 10);
        auto windowAmountKnob = AutoAttenuverter (this, PARAM_WINDOWAMOUNT, PARAM_WINDOWAMOUNT_CV_ATTEN, 10);

        auto burstCountKnob = AutoAttenuverter (this, PARAM_BURSTCOUNT, PARAM_BURSTCOUNT_CV_ATTEN, 10);
        auto restCountKnob = AutoAttenuverter (this, PARAM_RESTCOUNT, PARAM_RESTCOUNT_CV_ATTEN, 10);

        auto formantCVAtten = getParam (PARAM_FORMANT_CV_ATTEN);

        auto channelCount = getChannelCount ();
        setOutputChannels (OUTPUT_MAIN, channelCount);
        setOutputChannels (OUTPUT_REST, channelCount);
        engine.setChannelCount (channelCount);

        for (uint8_t channel = 0; channel < channelCount; channel++) {
            // Masking mode
            auto burstCount = burstCountKnob.process (getInputPoly (INPUT_BURSTCOUNT_CV, channel));
            auto restCount = restCountKnob.process (getInputPoly (INPUT_RESTCOUNT_CV, channel));
            switch (maskingMode) {
                case PulsarMaskingMode::Burst:
                    engine.setMaskingBurst (channel, burstToInt (burstCount * 6) + 1, burstToInt (restCount * 7));
                    break;

                case PulsarMaskingMode::Stochastic:
                    engine.setMaskingStochastic (channel, 1.f - burstCount, 1.f - restCount);
                    break;

                default: break;
            }

            // Calculate the frequencies
            auto maxFreq = std::min (MaxFreqHz, curSampleRate / 2.f);

            auto vOct = getParam (PARAM_FREQUENCY) + fpClean (getInputPoly (INPUT_VOCT, channel));
            auto baseFreq = std::clamp (rack::dsp::FREQ_C4 * rack::dsp::exp2_taylor5 (vOct), MinFreqHz, maxFreq);

            // Emission parameters
            engine.setEmissionFrequency (channel, baseFreq);

            // Pulsar parameters
            auto formantFreq = baseFreq;
            auto formantCV = getInputPoly (INPUT_FORMANT_CV, channel);
            if (!formantDecoupled) {
                auto formant = formantKnob.process (formantCV);
                formantFreq = rack::simd::fmin (baseFreq / Math::rescale1 (formant, 1.f, 1e-4f), maxFreq);
            } else {
                formantFreq = decoupledFormantFromParam (formantKnob.getParamValue (), formantCV * formantCVAtten);
                formantFreq = rack::simd::clamp (formantFreq, MinFreqHz, MaxFreqHz);
            }

            pulsarParams.frequency = formantFreq;
            pulsarParams.cluster = clusterKnob.process (getInputPoly (INPUT_CLUSTER_CV, channel));

            pulsarParams.shapeIndex = waveIndexKnob.process (getInputPoly (INPUT_WAVEINDEX_CV, channel));
            pulsarParams.shaperAmount = waveshaperKnob.process (getInputPoly (INPUT_WAVESHAPER_CV, channel));

            pulsarParams.windowIndex = windowIndexKnob.process (getInputPoly (INPUT_WINDOWINDEX_CV, channel));
            pulsarParams.windowSkew = windowSkewKnob.process (getInputPoly (INPUT_WINDOWSKEW_CV, channel));
            pulsarParams.windowAmount = windowAmountKnob.process (getInputPoly (INPUT_WINDOWAMOUNT_CV, channel));

            engine.setParams (channel, pulsarParams);
        }
    }

    void PulsarModule::process (const ProcessArgs& args) {
        auto& mainOutput = outputs [OUTPUT_MAIN];
        auto& restOutput = outputs [OUTPUT_REST];

        if (clockOversample.process ())
            updateOversampleRate ();

        // Update parameters
        engine.setActiveOutputs (mainOutput.isConnected (), restOutput.isConnected ());
        if (clockParams.process ())
            updateParams ();

        // Process the engine
        PulsarProcessArgs engineArgs;

        engineArgs.sampleRate = args.sampleRate;
        engineArgs.sampleTime = args.sampleTime;

        engineArgs.edgeFactor = getParam (PARAM_EDGE_FACTOR);

        if (isInputConnected (INPUT_SYNC)) {
            engineArgs.syncEnabled = true;

            if (isInputMonophonic (INPUT_SYNC))
                std::fill_n (engineArgs.syncVoltages, Constants::MaxPolyphony, getInput (INPUT_SYNC));
            else
                readInput (INPUT_SYNC, engineArgs.syncVoltages);
        } else
            engineArgs.syncEnabled = false;

        engineArgs.mainOut = mainOutput.getVoltages ();
        engineArgs.restOut = restOutput.getVoltages ();

        engine.process (engineArgs);
    }
}