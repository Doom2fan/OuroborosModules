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

#include "PulsarEngine.hpp"

#include "../JsonUtils.hpp"
#include "../Math.hpp"
#include "../SampleChannel.hpp"
#include "../DSP/Waveshapers.hpp"

#include <AudioFile.h>

namespace OuroborosModules::Modules::Pulsar {
    /*
     * Signal manipulation
     */
    template<typename T>
    [[using gnu: always_inline, hot]]
    inline T softClip (T sample) {
        return DSP::Waveshapers::softClipApprox (sample);
        return rack::simd::ifelse (rack::simd::abs (sample) > 1, DSP::Waveshapers::softClipApprox (sample + rack::simd::sgn (sample)), sample);
    }

    /*
     * Wavetables
     */
    struct PulsarWavetables {
        DSP::Wavetable waves [WavesCount] = { };

        DSP::Wavetable windows [WindowsCount] = { };
    };

    static std::weak_ptr<PulsarWavetables> wavetablesList = { };

    void loadWavetable (std::string path, DSP::Wavetable& table) {
        table = { };

        // Get the absolute path
        auto absPath = rack::asset::plugin (pluginInstance, path);

        // Load the audio file
        std::shared_ptr<Audio::AudioSample> sample;
        auto loadResult = Audio::AudioSample::load (absPath, -1, sample);

        if (loadResult != Audio::AudioSample::LoadStatus::Success) {
            LOG_WARN (FMT_STRING ("Error loading Pulsar wavetable: {}"), getErrorMessage (loadResult, path));
            return;
        }

        // Validate and calculate info
        if (sample->getChannelCount () != 1) {
            LOG_WARN (FMT_STRING ("Error loading Pulsar wavetable \"{}\": File is not mono."), path);
            return;
        }

        auto& rawBuffer = sample->getRawBuffer ();
        if ((rawBuffer.getSampleCount () % WavetableLength) != 0) {
            LOG_WARN (FMT_STRING ("Error loading Pulsar wavetable \"{}\": Sample count is not a multiple of {}."),
                path, WavetableLength);
            return;
        }

        auto frameCount = rawBuffer.getSampleCount () / WavetableLength;
        table.setSamples (rawBuffer.getSamples ().data (), WavetableLength, frameCount);
    }

    std::shared_ptr<PulsarWavetables> getWavetables () {
        if (auto wavetables = wavetablesList.lock ())
            return wavetables;

        auto wavetables = std::make_shared<PulsarWavetables> ();
        // Waves
        loadWavetable ("res/wavetables/PulsarSine.wav",     wavetables->waves [0]);
        loadWavetable ("res/wavetables/PulsarTriangle.wav", wavetables->waves [1]);
        loadWavetable ("res/wavetables/PulsarSaw.wav",      wavetables->waves [2]);
        loadWavetable ("res/wavetables/PulsarSquare.wav",   wavetables->waves [3]);
        // Last wave shape is "noise", for simplicitly we just make it an empty wavetable.
        float emptySamples [WavetableLength] = { };
        wavetables->waves [4].setSamples (emptySamples, WavetableLength, 1, 1);

        // Windows
        loadWavetable ("res/wavetables/PulsarWindowCosine.wav", wavetables->windows [0]);
        loadWavetable ("res/wavetables/PulsarWindowLinear.wav", wavetables->windows [1]);
        loadWavetable ("res/wavetables/PulsarWindowExp.wav",    wavetables->windows [2]);
        loadWavetable ("res/wavetables/PulsarWindowLog.wav",    wavetables->windows [3]);

        wavetablesList = wavetables;
        return wavetables;
    }

    [[using gnu: always_inline, hot]]
    inline uint32_t calcOctave (const DSP::Wavetable& wavetable, float phaseIncrement, bool oversampled) {
        auto value = wavetable.calculateOctave (phaseIncrement);
        return static_cast<uint32_t> (oversampled ? std::floor (value) : std::ceil (value));
    }

    struct WaveSampleData {
        const DSP::Wavetable* wavetable;

        uint32_t frameIndex;
        uint32_t octave;

        [[using gnu: always_inline, hot]]
        WaveSampleData () { }

        [[using gnu: always_inline, hot]]
        WaveSampleData (const DSP::Wavetable* wavetable, float frame, uint32_t octave)
            : wavetable (wavetable), octave (octave) {
            auto frameCount = wavetable->getFrameCount ();

            frameIndex = std::min (static_cast<uint32_t> (frame * (frameCount - 1)), frameCount - 1);
        }
    };

    template<typename TInterpolator>
    [[using gnu: always_inline, hot]]
    inline float sampleWave (const WaveSampleData& sampleData, float phase) {
        auto sampleIndex = std::clamp (phase * (WavetableLength - 1), 0.f, static_cast<float> (WavetableLength - 1));
        return sampleData.wavetable->getSample<TInterpolator> (sampleData.frameIndex, sampleIndex, sampleData.octave);
    }

    /*
     * PulsarMaskingData
     */
    json_t* PulsarMaskingData::dataToJson () const {
        auto rootJ = json_object ();

        json_object_set_new_enum (rootJ, "mode", mode);
        switch (mode) {
            case PulsarMaskingMode::Burst:
                json_object_set_new_int (rootJ, "burstCount", burstData.burstCount);
                json_object_set_new_int (rootJ, "restCount", burstData.restCount);

                json_object_set_new_int (rootJ, "curCount", burstData.curCount);
                json_object_set_new_bool (rootJ, "isRest", burstData.isRest);

                break;

            case PulsarMaskingMode::Stochastic:
                json_object_set_new_float (rootJ, "restProbability", stochasticData.restProbability);
                json_object_set_new_float (rootJ, "skipProbability", stochasticData.skipProbability);
                break;

            default: break;
        }

        return rootJ;
    }

    bool PulsarMaskingData::dataFromJson (json_t* rootJ) {
        if (!json_is_object (rootJ))
            return false;

        if (!json_object_try_get_enum (rootJ, "mode", mode))
            mode = PulsarMaskingMode::Invalid;

        auto modeInvalid = false;
        switch (mode) {
            case PulsarMaskingMode::Burst:
                modeInvalid |= !json_object_try_get_int (rootJ, "burstCount", burstData.burstCount);
                modeInvalid |= !json_object_try_get_int (rootJ, "restCount", burstData.restCount);

                modeInvalid |= !json_object_try_get_int (rootJ, "curCount", burstData.curCount);
                modeInvalid |= !json_object_try_get_bool (rootJ, "isRest", burstData.isRest);

                break;

            case PulsarMaskingMode::Stochastic:
                modeInvalid |= !json_object_try_get_float (rootJ, "restProbability", stochasticData.restProbability);
                modeInvalid |= !json_object_try_get_float (rootJ, "skipProbability", stochasticData.skipProbability);
                break;

            default: break;
        }

        // Ensure the struct is valid
        if (modeInvalid)
            mode = PulsarMaskingMode::Invalid;
        if (mode == PulsarMaskingMode::Invalid)
            setBurst (1, 0);

        return true;
    }

    inline void PulsarMaskingData::setBurst (uint32_t burstCount, uint32_t restCount) {
        // Parameters
        burstData.burstCount = burstCount;
        burstData.restCount = restCount;

        // State
        if (mode != PulsarMaskingMode::Burst) {
            burstData.curCount = 0;
            burstData.isRest = false;
        }

        mode = PulsarMaskingMode::Burst;
    }

    inline void PulsarMaskingData::setStochastic (float restProb, float noneProb) {
        // Parameters
        stochasticData.restProbability = restProb;
        stochasticData.skipProbability = noneProb;

        mode = PulsarMaskingMode::Stochastic;
    }

    [[using gnu: always_inline, hot]]
    inline PulsarMask PulsarMaskingData::process () {
        switch (mode) {
            case PulsarMaskingMode::Burst: {
                auto maxCount = (burstData.isRest ? burstData.restCount : burstData.burstCount);

                if (maxCount == 0)
                    burstData.isRest = !burstData.isRest;

                auto ret = burstData.isRest ? PulsarMask::Rest : PulsarMask::Pulse;
                burstData.curCount++;
                if (burstData.curCount >= maxCount) {
                    burstData.curCount = 0;
                    burstData.isRest = !burstData.isRest;
                }

                return ret;
            }

            case PulsarMaskingMode::Stochastic:
                if (rack::random::uniform () > stochasticData.skipProbability)
                    return PulsarMask::None;
                return (rack::random::uniform () <= stochasticData.restProbability) ? PulsarMask::Pulse : PulsarMask::Rest;

            default: return PulsarMask::None; // Mode is invalid, do nothing (accessing any data could be UB)
        }
    }

    /*
     * PulsarDataStore
     */
    json_t* PulsarParameters::dataToJson () const {
        auto rootJ = json_object ();

        // Parameters
        json_object_set_new_bool (rootJ, "isRest", isRest);

        json_object_set_new_float (rootJ, "frequency", frequency);
        json_object_set_new_float (rootJ, "cluster", cluster);

        json_object_set_new_float (rootJ, "shapeIndex", shapeIndex);
        json_object_set_new_float (rootJ, "shaperAmount", shaperAmount);

        json_object_set_new_float (rootJ, "windowIndex", windowIndex);
        json_object_set_new_float (rootJ, "windowSkew", windowSkew);
        json_object_set_new_float (rootJ, "windowAmount", windowAmount);

        // State
        json_object_set_new_float (rootJ, "wavePhase", wavePhase);
        json_object_set_new_float (rootJ, "windowPhase", windowPhase);
        json_object_set_new_float (rootJ, "edgePhase", edgePhase);

        return rootJ;
    }

    bool PulsarParameters::dataFromJson (json_t* rootJ) {
        if (!json_is_object (rootJ))
            return false;

        // Parameters
        json_object_try_get_bool (rootJ, "isRest", isRest);

        json_object_try_get_float (rootJ, "frequency", frequency);
        json_object_try_get_float (rootJ, "cluster", cluster);

        json_object_try_get_float (rootJ, "shapeIndex", shapeIndex);
        json_object_try_get_float (rootJ, "shaperAmount", shaperAmount);

        json_object_try_get_float (rootJ, "windowIndex", windowIndex);
        json_object_try_get_float (rootJ, "windowSkew", windowSkew);
        json_object_try_get_float (rootJ, "windowAmount", windowAmount);

        // State
        json_object_try_get_float (rootJ, "wavePhase", wavePhase);
        json_object_try_get_float (rootJ, "windowPhase", windowPhase);
        json_object_try_get_float (rootJ, "edgePhase", edgePhase);

        return true;
    }

    /*
     * PulsarDataStore
     */
    json_t* PulsarDataStore::dataToJson () const {
        auto rootJ = json_object ();

        auto slotUsedJ = json_array ();
        for (size_t i = 0; i < SlotBankCount; i++)
            json_array_append_new (slotUsedJ, json_integer (slotUsed [i]));
        json_object_set_new (rootJ, "slotUsed", slotUsedJ);

        auto isRestJ = json_array ();
        auto pulsarsJ = json_array ();
        for (size_t i = 0; i < MaxPulsars; i++) {
            json_array_append_new (isRestJ, json_boolean (isRest [i]));
            json_array_append_new (pulsarsJ, slotToParams (i).dataToJson ());
        }
        json_object_set_new (rootJ, "isRest", isRestJ);
        json_object_set_new (rootJ, "pulsars", pulsarsJ);

        return rootJ;
    }

    bool PulsarDataStore::dataFromJson (json_t* rootJ) {
        if (!json_is_object (rootJ))
            return false;

        // Fetch the arrays
        auto slotUsedJ = json_object_get (rootJ, "slotUsed");
        if (!json_is_array (slotUsedJ))
            return false;

        auto isRestJ = json_object_get (rootJ, "isRest");
        if (!json_is_array (isRestJ))
            return false;

        auto pulsarsJ = json_object_get (rootJ, "pulsars");
        if (!json_is_array (pulsarsJ))
            return false;

        // Fail if the arrays aren't the same size
        auto slotUsedLength = json_array_size (slotUsedJ);
        auto pulsarsLength = json_array_size (pulsarsJ);
        if (slotUsedLength * SlotBankSize != pulsarsLength)
            return false;

        for (size_t i = 0; i < slotUsedLength; i++) {
            auto slotJ = json_array_get (slotUsedJ, i);
            if (!json_is_integer (slotJ))
                return false;

            if (i < SlotBankCount)
                slotUsed [i] = json_integer_value (slotJ);
        }

        if (slotUsedLength < SlotBankCount)
            std::fill (std::begin (slotUsed) + slotUsedLength, std::end (slotUsed), 0);

        for (size_t i = 0; i < pulsarsLength; i++) {
            auto restValJ = json_array_get (isRestJ, i);
            auto pulsarJ = json_array_get (pulsarsJ, i);

            if (!json_is_boolean (restValJ))
                return false;

            PulsarParameters pulsar;
            if (!pulsar.dataFromJson (pulsarJ))
                return false;
            copyToSlot (i, pulsar);
        }

        return true;
    }

    [[gnu::hot]]
    void PulsarDataStore::setSlot (uint32_t index, bool used) {
        assert (index < MaxPulsars);
        if (index >= MaxPulsars)
            return;

        auto bank = index / SlotBankSize;
        auto bit = 1 << (index % SlotBankSize);
        slotUsed [bank] = (slotUsed [bank] & ~bit) | (used ? bit : 0);
    }

    [[gnu::hot]]
    void PulsarDataStore::copyToSlot (uint32_t index, const PulsarParameters& params) {
        // Parameters
        isRest [index] = params.isRest;

        frequency [index] = params.frequency;
        cluster [index] = params.cluster;

        waveIndex [index] = std::min (static_cast<uint32_t> (params.shapeIndex), WavesCount - 2);
        waveIndexFrac [index] = std::clamp (params.shapeIndex - waveIndex [index], 0.f, 1.f);
        shaperAmount [index] = params.shaperAmount;

        windowIndex [index] = std::min (static_cast<uint32_t> (params.windowIndex), WindowsCount - 2);
        windowIndexFrac [index] = std::clamp (params.windowIndex - windowIndex [index], 0.f, 1.f);
        windowSkew [index] = params.windowSkew;
        windowAmount [index] = params.windowAmount;

        // State
        wavePhase [index] = params.wavePhase;
        windowPhase [index] = params.windowPhase;
        edgePhase [index] = params.edgePhase;
    }

    void PulsarDataStore::slotToParams (uint32_t index, PulsarParameters& params) const {
        // Parameters
        params.isRest = isRest [index];

        params.frequency = frequency [index];
        params.cluster = cluster [index];

        params.shapeIndex = waveIndex [index] + waveIndexFrac [index];
        params.shaperAmount = shaperAmount [index];

        params.windowIndex = windowIndex [index] + windowIndexFrac [index];
        params.windowSkew = windowSkew [index];
        params.windowAmount = windowAmount [index];

        // State
        params.wavePhase = wavePhase [index];
        params.windowPhase = windowPhase [index];
        params.edgePhase = edgePhase [index];
    }

    /*
     * NoiseBuffer
     */
    NoiseBuffer::NoiseBuffer () {
        // Fill the buffer with noise
        for (uint32_t i = 0; i < BufferSize; i++)
            buffer [i] = rack::random::uniform () * 2.f - 1.f;
    }

    void NoiseBuffer::update () {
        readIdx = rack::random::u32 () & BufferSizeMask;

        buffer [writeIdx] = rack::random::uniform () * 2.f - 1.f;
        writeIdx = (writeIdx + 1) & BufferSizeMask;
    }

    float NoiseBuffer::read () {
        auto idx = readIdx++;
        readIdx &= BufferSizeMask;
        return buffer [idx];
    }

    void NoiseBuffer::readCount (float* outBuffer, uint32_t count) {
        while (count > 0) {
            outBuffer [--count] = buffer [readIdx];
            readIdx = (readIdx + 1) & BufferSizeMask;
        }
    }

    /*
     * PulsarEngine
     */
    PulsarEngine::PulsarEngine () {
        wavetables = getWavetables ();

        setOversampling (DefaultOversampleRate, true);
    }

    json_t* PulsarEngine::channelToJson (uint32_t channel) const {
        auto rootJ = json_object ();

        // Parameters
        json_object_set_new_struct (rootJ, "params", parameters [channel]);

        json_object_set_new_float (rootJ, "emissionFrequency", emissionFrequency [channel]);

        // State
        json_object_set_new_struct (rootJ, "pulsars", pulsars [channel]);
        json_object_set_new_struct (rootJ, "maskingData", maskingData [channel]);

        return rootJ;
    }

    bool PulsarEngine::channelFromJson (json_t* rootJ, uint32_t channel) {
        if (!json_is_object (rootJ))
            return false;

        auto failed = false;

        // Parameters
        failed |= !json_object_try_get_struct (rootJ, "params", parameters [channel]);

        failed |= !json_object_try_get_float (rootJ, "emissionFrequency", emissionFrequency [channel]);

        // State
        failed |= !json_object_try_get_struct (rootJ, "pulsars", pulsars [channel]);
        failed |= !json_object_try_get_struct (rootJ, "maskingData", maskingData [channel]);

        if (failed)
            return false;

        return true;
    }

    json_t* PulsarEngine::dataToJson () const {
        auto rootJ = json_object ();

        auto channelsJ = json_array ();
        for (int channel = 0; channel < Constants::MaxPolyphony; channel++)
            json_array_append (channelsJ, channelToJson (channel));
        json_object_set_new (rootJ, "channels", channelsJ);

        return rootJ;
    }

    bool PulsarEngine::dataFromJson (json_t* rootJ) {
        if (!json_is_object (rootJ))
            return false;

        auto failed = false;

        auto channelsJ = json_object_get (rootJ, "channels");
        if (!json_is_array (channelsJ))
            failed |= true;
        else {
            auto channelCount = std::min (json_array_size (channelsJ), static_cast<size_t> (Constants::MaxPolyphony));

            uint32_t channel = 0;
            for (; channel < channelCount; channel++)
                failed |= !channelFromJson (json_array_get (channelsJ, channel), channel);

            for (; channel < Constants::MaxPolyphony; channel++) {
                parameters [channel] = PulsarParameters ();
                maskingData [channel] = PulsarMaskingData ();
            }
        }

        if (failed) {
            auto sampleRate = curSampleRate;
            *this = PulsarEngine ();
            curSampleRate = sampleRate;

            return false;
        }

        updatePulsarOctaves (curSampleRate);

        return true;
    }

    void PulsarEngine::setOversampling (uint8_t factor, bool force) {
        assert (factor <= MaxOversample);

        if (!force && oversampleFactor == factor)
            return;
        if (factor > MaxOversample)
            return;

        oversampleFactor = factor;
        for (uint32_t bank = 0; bank < SIMDBankCount; bank++) {
            decimatorMain [bank].setParams (factor);
            decimatorRest [bank].setParams (factor);
        }

        updatePulsarOctaves (curSampleRate);
    }

    void PulsarEngine::setOverlapMode (bool overlap) {
        overlapMode = overlap;
    }

    void PulsarEngine::setTriggeredMode (bool enable) {
        if (triggeredMode != enable) {
            for (uint32_t channel = 0; channel < Constants::MaxPolyphony; channel++)
                emissionPhase [channel] = 0.f;
        }

        triggeredMode = enable;
    }

    void PulsarEngine::setEmissionFrequency (uint32_t channel, float freq) {
        emissionFrequency [channel] = freq;
    }

    void PulsarEngine::setParams (uint32_t channel, const PulsarParameters& params) {
        parameters [channel] = params;
    }

    void PulsarEngine::setMaskingBurst (uint32_t channel, uint32_t burstCount, uint32_t restCount) {
        maskingData [channel].setBurst (burstCount, restCount);
    }

    void PulsarEngine::setMaskingStochastic (uint32_t channel, float restProbability, float noneProbability) {
        maskingData [channel].setStochastic (restProbability, noneProbability);
    }

    void PulsarEngine::setChannelCount (uint32_t count) {
        channelCount = std::clamp (count, 1u, static_cast<uint32_t> (Constants::MaxPolyphony));
    }

    void PulsarEngine::setActiveOutputs (bool main, bool rest) {
        outputActiveMain = main;
        outputActiveRest = rest;
    }

    [[gnu::hot]]
    void PulsarEngine::emitPulsar (uint32_t channel, const PulsarParameters& params, bool isRest, float emissionPhase) {
        auto& pulsars = this->pulsars [channel];

        // Don't emit pulsars if the respective outputs aren't active
        if ((!isRest && !outputActiveMain) || (isRest && !outputActiveRest))
            return;

        uint32_t pulsarIndex = MaxPulsars;
        for (uint32_t i = 0; i < SlotBankCount; i++) {
            auto idx = __builtin_ffs (~pulsars.slotUsed [i] & 0x00FF);
            if (idx > 0) {
                pulsarIndex = i * SlotBankSize + (idx - 1);
                break;
            }
        }

        // All slots occupied, can't emit pulsar
        if (pulsarIndex >= MaxPulsars)
            return;

        pulsars.setSlot (pulsarIndex, true);
        pulsars.copyToSlot (pulsarIndex, params);

        pulsars.isRest [pulsarIndex] = isRest ? 1.f : 0.f;
        pulsars.wavePhase [pulsarIndex] = emissionPhase * params.cluster;
        pulsars.windowPhase [pulsarIndex] = emissionPhase;
        pulsars.edgePhase [pulsarIndex] = -1.f;

        setPulsarOctave (channel, pulsarIndex);

        if (!overlapMode) {
            for (uint32_t i = 0; i < MaxPulsars; i++) {
                if (i != pulsarIndex && pulsars.isRest [i] == isRest)
                    pulsars.edgePhase [i] = 0.f;
            }
        }
    }

    [[gnu::hot]]
    void PulsarEngine::processPulsarQuad (PulsarFrameArgs& args) {
        using LinearInterp = DSP::WavetableInterpolators::Linear;

        auto osFactor = oversampleFactor;

        auto slotMask = args.slotMask;
        auto baseIndex = args.baseIndex;

        auto& pulsars = *args.pulsars;
        auto& wavetables = *args.wavetables;

        float wave0Arr [SIMDBankSize];
        float wave1Arr [SIMDBankSize];
        float window0Arr [SIMDBankSize];
        float window1Arr [SIMDBankSize];
        float noiseArr [SIMDBankSize];
        noiseBuffer.readCount (noiseArr, SIMDBankSize);

        // Parameters
        auto freq = VectorT::load (pulsars.frequency + args.baseIndex);
        auto windowAmount = VectorT::load (pulsars.windowAmount + args.baseIndex);
        auto restMask = VectorT::load (pulsars.isRest + args.baseIndex) >= .5f;

        auto waveIndexFrac = VectorT::load (pulsars.waveIndexFrac + args.baseIndex);
        auto windowIndexFrac = VectorT::load (pulsars.windowIndexFrac + args.baseIndex);

        // State
        auto usedMask = rack::simd::movemaskInverse<VectorT> (slotMask);
        auto wavePhase = VectorT::load (pulsars.wavePhase + args.baseIndex);
        auto windowPhase = VectorT::load (pulsars.windowPhase + args.baseIndex);
        auto edgePhase = VectorT::load (pulsars.edgePhase + args.baseIndex);

        auto wavePhaseIncrement = (freq * VectorT::load (pulsars.cluster + args.baseIndex) * args.osSampleTime) & usedMask;
        auto windowPhaseIncrement = (freq * args.osSampleTime) & usedMask;
        auto edgePhaseIncrement = (VectorT (args.edgeFrequency) * args.osSampleTime) & (edgePhase >= 0.f) & usedMask;

        WaveSampleData wave0Data [SIMDBankSize];
        WaveSampleData wave1Data [SIMDBankSize];
        WaveSampleData window0Data [SIMDBankSize];
        WaveSampleData window1Data [SIMDBankSize];

        for (uint32_t i = 0; i < SIMDBankSize; i++) {
            auto slotIdx = baseIndex + i;

            auto wave0 = pulsars.waveIndex [slotIdx];
            auto frame = pulsars.shaperAmount [slotIdx];
            wave0Data [i] = WaveSampleData (&wavetables.waves [wave0    ], frame, pulsars.wave0Octave [slotIdx]);
            wave1Data [i] = WaveSampleData (&wavetables.waves [wave0 + 1], frame, pulsars.wave1Octave [slotIdx]);

            auto window0 = pulsars.windowIndex [slotIdx];
            frame = pulsars.windowSkew [slotIdx];
            window0Data [i] = WaveSampleData (&wavetables.windows [window0    ], frame, pulsars.window0Octave [slotIdx]);
            window1Data [i] = WaveSampleData (&wavetables.windows [window0 + 1], frame, pulsars.window1Octave [slotIdx]);
        }

        for (int i = 0; i < osFactor; i++) {
            // Generate signal and window
            for (uint32_t j = 0; j < SIMDBankSize; j++) {
                if ((slotMask & (1 << j)) == 0)
                    continue;

                auto phase = wavePhase [j];
                wave0Arr [j] = sampleWave<LinearInterp> (wave0Data [j], phase);
                if ((pulsars.waveIndex [baseIndex + j] + 1) < WavesCount - 1)
                    wave1Arr [j] = sampleWave<LinearInterp> (wave1Data [j], phase);
                else
                    wave1Arr [j] = noiseArr [j];

                phase = windowPhase [j];
                window0Arr [j] = sampleWave<LinearInterp> (window0Data [j], phase);
                window1Arr [j] = sampleWave<LinearInterp> (window1Data [j], phase);
            }

            // Load and crossfade signals
            auto signal = Math::lerp (VectorT::load (wave0Arr), VectorT::load (wave1Arr), waveIndexFrac);
            auto windowSignal = Math::lerp (VectorT::load (window0Arr), VectorT::load (window1Arr), windowIndexFrac);

            // Apply windowing
            signal *= Math::lerp (VectorT (1.f), windowSignal, windowAmount);
            signal *= 1.f - rack::simd::clamp (edgePhase, 0.f, 1.f);

            // Advance phase
            wavePhase += wavePhaseIncrement;
            windowPhase += windowPhaseIncrement;
            edgePhase += edgePhaseIncrement;

            wavePhase -= rack::simd::floor (wavePhase);

            // Accumulate the signal
            signal &= usedMask;
            args.mainSignal [i] += signal & ~restMask;
            args.restSignal [i] += signal & restMask;
        }

        // Store phase
        wavePhase.store (pulsars.wavePhase + baseIndex);
        windowPhase.store (pulsars.windowPhase + baseIndex);
        edgePhase.store (pulsars.edgePhase + baseIndex);

        // Update the used slots
        usedMask &= (windowPhase < 1) & (edgePhase < 1);
        args.slotMask &= rack::simd::movemask (usedMask) & 0x0F;
    }

    [[gnu::hot]]
    void PulsarEngine::processPulsars (PulsarProcessArgs& args, PulsarOutput& pulsarOut, uint32_t channel, uint32_t bankIdx) {
        auto& pulsars = this->pulsars [channel];
        auto osFactor = oversampleFactor;

        PulsarFrameArgs frameArgs = { };
        frameArgs.osSampleRate = args.sampleRate * osFactor;
        frameArgs.osSampleTime = 1.f / frameArgs.osSampleRate;
        frameArgs.edgeFrequency = 1.f / std::max (0.f, std::min (
            args.edgeFactor,
            std::floor (1.f / emissionFrequency [channel] * frameArgs.osSampleRate - 1) * frameArgs.osSampleTime
        ));
        frameArgs.pulsars = &pulsars;
        frameArgs.wavetables = args.wavetables;

        for (uint32_t slotBankIdx = 0; slotBankIdx < SlotBankCount; slotBankIdx++) {
            auto slotBank = pulsars.slotUsed [slotBankIdx];
            if (slotBank == 0)
                continue;

            auto slotBankBaseIdx = slotBankIdx * SlotBankSize;
            for (uint32_t i = 0; i < SlotBankSize / 4; i++) {
                frameArgs.slotMask = (slotBank >> (i * 4)) & 0x0F;
                if (frameArgs.slotMask == 0)
                    continue;

                frameArgs.baseIndex = slotBankBaseIdx + i * 4;
                processPulsarQuad (frameArgs);

                slotBank &= ~(0x0F << (i * 4));
                slotBank |= frameArgs.slotMask << (i * 4);
            }

            pulsars.slotUsed [slotBankIdx] = slotBank;
        }

        for (int i = 0; i < osFactor; i++) {
            pulsarOut.mainSignal [i] [bankIdx] = softClip (Math::hsum (frameArgs.mainSignal [i]));
            pulsarOut.restSignal [i] [bankIdx] = softClip (Math::hsum (frameArgs.restSignal [i]));
        }
    }

    [[gnu::hot]]
    void PulsarEngine::processPulsars (PulsarProcessArgs& args) {
        for (uint32_t channel = 0, simdBank = 0; channel < channelCount; channel += SIMDBankSize, simdBank++) {
            PulsarOutput output;

            auto bankSize = std::min (channelCount - channel, SIMDBankSize);
            for (uint32_t bankIndex = 0; bankIndex < bankSize; bankIndex++)
                processPulsars (args, output, channel + bankIndex, bankIndex);

            (decimatorMain [simdBank].process (output.mainSignal) * 5.f).store (args.mainOut + channel);
            (decimatorRest [simdBank].process (output.restSignal) * 5.f).store (args.restOut + channel);
        }
    }

    [[gnu::hot]]
    void PulsarEngine::processEmission (PulsarProcessArgs& args) {
        auto triggeredModeMask = !triggeredMode ? VectorT::mask () : VectorT::zero ();
        auto syncEnabledMask = args.syncEnabled ? VectorT::mask () : VectorT::zero ();

        for (uint32_t bank = 0, baseChannel = 0; baseChannel < channelCount; bank++, baseChannel += SIMDBankSize) {
            int emitMask = 0;

            auto deltaPhase = VectorT::load (emissionFrequency + baseChannel) * args.sampleTime;
            auto phase = VectorT::load (emissionPhase + baseChannel);

            phase += deltaPhase & triggeredModeMask;
            emitMask |= rack::simd::movemask ((phase & triggeredModeMask) >= 1.f);
            phase -= rack::simd::floor (phase) & triggeredModeMask;

            auto syncValue = VectorT::load (args.syncVoltages + baseChannel);
            auto deltaSync = syncValue - lastSyncValues [bank];
            auto syncCrossing = -lastSyncValues [bank] / deltaSync;
            lastSyncValues [bank] = syncValue;

            auto sync = (0.f < syncCrossing) & (syncCrossing <= 1.f) & (syncValue >= 0.f) & syncEnabledMask;
            phase = rack::simd::ifelse (sync, (1.f - syncCrossing) * deltaPhase, phase);
            emitMask |= rack::simd::movemask (sync);

            phase.store (emissionPhase + baseChannel);

            auto bankSize = std::min (channelCount - baseChannel, SIMDBankSize);
            for (uint32_t i = 0; i < bankSize; i++) {
                if (!(emitMask & (1 << i)))
                    continue;

                auto channel = baseChannel + i;
                auto mask = maskingData [channel].process ();
                if (mask != PulsarMask::None)
                    emitPulsar (channel, parameters [channel], mask == PulsarMask::Rest, phase [i]);
            }
        }
    }

    void PulsarEngine::process (PulsarProcessArgs& args) {
        args.wavetables = &(*this->wavetables);

        noiseBuffer.update ();

        processEmission (args);
        processPulsars (args);
    }

    [[gnu::hot]]
    void PulsarEngine::setPulsarOctave (uint32_t channel, uint32_t slot) {
        auto& pulsars = this->pulsars [channel];
        auto& wavetables = *(this->wavetables);
        auto oversampled = oversampleFactor > 1;

        auto normalizedFreq = pulsars.frequency [slot] / curSampleRate;
        auto waveFreq = normalizedFreq * pulsars.cluster [slot];

        auto wave0 = pulsars.waveIndex [slot];
        pulsars.wave0Octave [slot] = calcOctave (wavetables.waves [wave0], waveFreq, oversampled);
        pulsars.wave1Octave [slot] = calcOctave (wavetables.waves [wave0 + 1], waveFreq, oversampled);

        auto window0 = pulsars.windowIndex [slot];
        pulsars.window0Octave [slot] = calcOctave (wavetables.windows [window0], normalizedFreq, oversampled);
        pulsars.window1Octave [slot] = calcOctave (wavetables.windows [window0 + 1], normalizedFreq, oversampled);
    }

    void PulsarEngine::updatePulsarOctaves (float sampleRate) {
        for (uint32_t channel = 0; channel < Constants::MaxPolyphony; channel++) {
            auto& pulsars = this->pulsars [channel];

            for (uint32_t slotBankIdx = 0; slotBankIdx < SlotBankCount; slotBankIdx++) {
                auto slotBank = pulsars.slotUsed [slotBankIdx];
                if (slotBank == 0)
                    continue;

                auto slotBankBaseIdx = slotBankIdx * SlotBankSize;
                for (uint32_t i = 0; i < SlotBankSize; i++) {
                    if ((slotBank & (1 << i)) == 0)
                        continue;

                    setPulsarOctave (channel, slotBankBaseIdx + i);
                }
            }
        }
    }

    void PulsarEngine::onSampleRateChange (float newSampleRate) {
        if (newSampleRate == curSampleRate)
            return;

        curSampleRate = newSampleRate;
        updatePulsarOctaves (newSampleRate);
    }
}