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
        auto sign = rack::simd::sgn (sample);
        auto shaped = DSP::Waveshapers::softClipApprox (sample - sign) + sign;
        return rack::simd::ifelse (rack::simd::abs (sample) > 1, shaped, sample);
    }

    /*
     * Wavetables
     */
    static std::weak_ptr<PulsarWavetables> wavetablesList = { };

    void loadWavetable (std::string path, DSP::Wavetable& table, bool removeDC) {
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
        table.setSamples (rawBuffer.getSamples ().data (), WavetableLength, frameCount, 50000, removeDC);
    }

    std::shared_ptr<PulsarWavetables> getWavetables () {
        if (auto wavetables = wavetablesList.lock ())
            return wavetables;

        auto wavetables = std::make_shared<PulsarWavetables> ();
        // Waves
        loadWavetable ("res/wavetables/PulsarSine.wav",     wavetables->waves [0], true);
        loadWavetable ("res/wavetables/PulsarTriangle.wav", wavetables->waves [1], true);
        loadWavetable ("res/wavetables/PulsarSaw.wav",      wavetables->waves [2], true);
        loadWavetable ("res/wavetables/PulsarSquare.wav",   wavetables->waves [3], true);
        // Last wave shape is "noise", for simplicitly we just make it an empty wavetable
        float emptySamples [WavetableLength] = { };
        wavetables->waves [4].setSamples (emptySamples, WavetableLength, 1, 1);

        // Windows
        loadWavetable ("res/wavetables/PulsarWindowCosine.wav", wavetables->windows [0], false);
        loadWavetable ("res/wavetables/PulsarWindowLinear.wav", wavetables->windows [1], false);
        loadWavetable ("res/wavetables/PulsarWindowExp.wav",    wavetables->windows [2], false);
        loadWavetable ("res/wavetables/PulsarWindowLog.wav",    wavetables->windows [3], false);

        wavetablesList = wavetables;
        return wavetables;
    }

    [[using gnu: always_inline, hot]]
    inline uint32_t calcOctave (const DSP::Wavetable& wavetable, float phaseIncrement, bool oversampled) {
        auto value = wavetable.calculateOctave (phaseIncrement);
        return static_cast<uint32_t> (oversampled ? std::floor (value) : std::ceil (value));
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
        json_object_set_new_float (rootJ, "edgeFrequency", edgeFrequency);

        json_object_set_new_float (rootJ, "pulsarPhase", pulsarPhase);
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
        json_object_try_get_float (rootJ, "edgeFrequency", edgeFrequency);

        json_object_try_get_float (rootJ, "pulsarPhase", pulsarPhase);
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

        auto pulsarsJ = json_array ();
        for (size_t i = 0; i < MaxPulsars; i++)
            json_array_append_new (pulsarsJ, slotToParams (i).dataToJson ());

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
            auto pulsarJ = json_array_get (pulsarsJ, i);

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
        edgeFrequency [index] = params.edgeFrequency;

        pulsarPhase [index] = params.pulsarPhase;
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
        params.edgeFrequency = edgeFrequency [index];

        params.pulsarPhase = pulsarPhase [index];
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

        json_object_set_new_float (rootJ, "emissionPhase", emissionPhase [channel]);

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

        failed |= !json_object_try_get_float (rootJ, "emissionPhase", emissionPhase [channel]);

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
            syncUpsampler [bank].setParams (factor);
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
    void PulsarEngine::emitPulsar (PulsarProcessArgs& args, uint32_t channel, bool isRest, int curSample) {
        // Don't emit pulsars if the respective outputs aren't active
        if ((!isRest && !outputActiveMain) || (isRest && !outputActiveRest))
            return;

        auto& pulsars = this->pulsars [channel];

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

        const auto& params = parameters [channel];

        auto emissionPhase = this->emissionPhase [channel];
        auto emissionFrequency = this->emissionFrequency [channel];
        auto emissionFrequencyInv = 1.f / emissionFrequency;

        auto sampleOffset = static_cast<float> (curSample) / oversampleFactor;
        auto deltaPhase = params.frequency / args.sampleRate;
        auto basePhase = -sampleOffset * deltaPhase + emissionPhase * params.frequency * emissionFrequencyInv;

       // Parameters
        pulsars.setSlot (pulsarIndex, true);
        pulsars.copyToSlot (pulsarIndex, params);

        pulsars.isRest [pulsarIndex] = isRest ? 1.f : 0.f;

        // State
        pulsars.edgeFrequency [pulsarIndex] = 0.f;

        pulsars.pulsarPhase [pulsarIndex] = basePhase;
        pulsars.edgePhase [pulsarIndex] = 0.f;

        setPulsarOctave (channel, pulsarIndex);

        if (!overlapMode) {
            auto oneSampleHz = args.sampleRate;
            auto baseEdgeFreq = std::max (args.edgeFactor, emissionFrequency);

            auto restMask = isRest ? VectorT::mask () : VectorT::zero ();
            for (uint32_t baseIndex = 0; baseIndex < MaxPulsars; baseIndex += SIMDBankSize) {
                auto slot = baseIndex + VectorT (0, 1, 2, 3);

                // Get parameters
                auto isRest = VectorT::load (pulsars.isRest + baseIndex) >= .5f;
                auto edgeFrequency = VectorT::load (pulsars.edgeFrequency + baseIndex);
                auto edgePhase = VectorT::load (pulsars.edgePhase + baseIndex);

                // Calculate slot mask
                auto slotMask = (slot != pulsarIndex) & (isRest == restMask) & (edgeFrequency <= 0.f);

                // Calculate and set edge frequency and phase
                auto phaseLeft = 1 - rack::simd::clamp (VectorT::load (pulsars.pulsarPhase + baseIndex), 0, 1);
                auto phaseFrequency = 1.f / rack::simd::fmin (1e-15f, phaseLeft);

                auto newEdgeFreq = rack::simd::fmin (oneSampleHz, rack::simd::fmax (baseEdgeFreq, phaseFrequency));
                auto edgeDelta = newEdgeFreq * args.sampleTime;
                auto newEdgePhase = -sampleOffset * edgeDelta + emissionPhase * newEdgeFreq * emissionFrequencyInv;

                edgeFrequency = rack::simd::ifelse (slotMask, newEdgeFreq, edgeFrequency);
                edgePhase = rack::simd::ifelse (slotMask, newEdgePhase, edgePhase);

                edgeFrequency.store (pulsars.edgeFrequency + baseIndex);
                edgePhase.store (pulsars.edgePhase + baseIndex);
            }
        }
    }

    [[gnu::hot]]
    void PulsarEngine::processPulsarQuad (PulsarFrameArgs& args) {
        using rack::simd::int32_4;
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
        auto noiseVec = VectorT::load (noiseArr);

        // Parameters
        auto freq = VectorT::load (pulsars.frequency + baseIndex);
        auto cluster = VectorT::load (pulsars.cluster + baseIndex);
        auto windowAmount = VectorT::load (pulsars.windowAmount + baseIndex);
        auto restMask = VectorT::load (pulsars.isRest + baseIndex) >= .5f;

        auto waveIndexFrac = VectorT::load (pulsars.waveIndexFrac + baseIndex);
        auto windowIndexFrac = VectorT::load (pulsars.windowIndexFrac + baseIndex);

        auto noiseMask = (VectorT) (int32_4::load ((int32_t*) (pulsars.waveIndex + baseIndex)) + 1) >= WavesCount - 1;

        // State
        auto usedMask = rack::simd::movemaskInverse<VectorT> (slotMask);
        auto pulsarPhase = VectorT::load (pulsars.pulsarPhase + baseIndex);
        auto edgePhase = VectorT::load (pulsars.edgePhase + baseIndex);

        auto pulsarPhaseIncrement = (freq * args.osSampleTime) & usedMask;
        auto edgePhaseIncrement = (VectorT::load (pulsars.edgeFrequency + baseIndex) * args.osSampleTime) & usedMask;

        DSP::WavetableSampler wave0Sampler [SIMDBankSize];
        DSP::WavetableSampler wave1Sampler [SIMDBankSize];
        DSP::WavetableSampler window0Sampler [SIMDBankSize];
        DSP::WavetableSampler window1Sampler [SIMDBankSize];

        uint32_t slotCount = 0;
        uint32_t slotIndices [SIMDBankSize] = { };
        for (uint32_t j = 0; j < SIMDBankSize; j++) {
            if ((slotMask & (1 << j)) == 0)
                continue;

            slotIndices [slotCount++] = j;
        }

        for (uint32_t i = 0; i < slotCount; i++) {
            auto slot = slotIndices [i];
            auto slotIdx = baseIndex + slot;

            auto wave0 = pulsars.waveIndex [slotIdx];
            auto frame = pulsars.shaperAmount [slotIdx];
            wave0Sampler [slot] = getSampler (&wavetables.waves [wave0    ], frame, pulsars.wave0Octave [slotIdx]);
            wave1Sampler [slot] = getSampler (&wavetables.waves [wave0 + 1], frame, pulsars.wave1Octave [slotIdx]);

            auto window0 = pulsars.windowIndex [slotIdx];
            frame = pulsars.windowSkew [slotIdx];
            window0Sampler [slot] = getSampler (&wavetables.windows [window0    ], frame, pulsars.window0Octave [slotIdx]);
            window1Sampler [slot] = getSampler (&wavetables.windows [window0 + 1], frame, pulsars.window1Octave [slotIdx]);
        }

        int32_t waveIndices [SIMDBankSize], windowIndices [SIMDBankSize];
        float waveFracs [SIMDBankSize], windowFracs [SIMDBankSize];

        for (uint32_t sampleIdx = 0; sampleIdx < osFactor; sampleIdx++) {
            auto windowPhase = rack::simd::clamp (pulsarPhase, 0, 1);
            auto wavePhase = windowPhase * cluster;
            wavePhase -= rack::simd::floor (wavePhase);

            calcSampleIndex (wavePhase, waveIndices, waveFracs);
            calcSampleIndex (windowPhase, windowIndices, windowFracs);

            // Generate signal and window
            for (uint32_t j = 0; j < slotCount; j++) {
                auto slot = slotIndices [j];

                auto sampleIndex = static_cast<uint32_t> (waveIndices [slot]);
                auto sampleFrac = waveFracs [slot];
                wave0Arr [slot] = wave0Sampler [slot].sampleFrac<LinearInterp> (sampleIndex, sampleFrac);
                wave1Arr [slot] = wave1Sampler [slot].sampleFrac<LinearInterp> (sampleIndex, sampleFrac);

                sampleIndex = static_cast<uint32_t> (windowIndices [slot]);
                sampleFrac = windowFracs [slot];
                window0Arr [slot] = window0Sampler [slot].sampleFrac<LinearInterp> (sampleIndex, sampleFrac);
                window1Arr [slot] = window1Sampler [slot].sampleFrac<LinearInterp> (sampleIndex, sampleFrac);
            }

            // Load and crossfade signals
            auto wave1 = rack::simd::ifelse (noiseMask, noiseVec, VectorT::load (wave1Arr));
            auto signal = Math::lerp (VectorT::load (wave0Arr), wave1, waveIndexFrac);
            auto windowSignal = Math::lerp (VectorT::load (window0Arr), VectorT::load (window1Arr), windowIndexFrac);

            // Apply windowing
            signal *= Math::lerp (VectorT (1.f), windowSignal, windowAmount);
            signal *= 1.f - rack::simd::clamp (edgePhase, 0, 1);

            // Accumulate the signal
            signal &= usedMask;
            args.mainSignal [sampleIdx] += signal & ~restMask;
            args.restSignal [sampleIdx] += signal & restMask;

            // Advance phase
            pulsarPhase += pulsarPhaseIncrement;
            edgePhase += edgePhaseIncrement;

            usedMask &= (pulsarPhase <= 1) & (edgePhase <= 1);
        }

        // Store phase
        pulsarPhase.store (pulsars.pulsarPhase + baseIndex);
        edgePhase.store (pulsars.edgePhase + baseIndex);

        // Update the used slots
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
            std::floor (frameArgs.osSampleRate / emissionFrequency [channel] - 1) * frameArgs.osSampleTime
        ));

        frameArgs.pulsars = &pulsars;
        frameArgs.wavetables = args.wavetables;

        for (uint32_t slotBankIdx = 0; slotBankIdx < SlotBankCount; slotBankIdx++) {
            auto slotBank = pulsars.slotUsed [slotBankIdx];
            if (slotBank == 0)
                continue;

            auto slotBankBaseIdx = slotBankIdx * SlotBankSize;
            for (uint32_t i = 0; i < SlotBankSize / SIMDBankSize; i++) {
                frameArgs.slotMask = (slotBank >> (i * SIMDBankSize)) & 0x0F;
                if (frameArgs.slotMask == 0)
                    continue;

                frameArgs.baseIndex = slotBankBaseIdx + i * SIMDBankSize;
                processPulsarQuad (frameArgs);

                slotBank &= ~(0x0F << (i * SIMDBankSize));
                slotBank |= frameArgs.slotMask << (i * SIMDBankSize);
            }

            pulsars.slotUsed [slotBankIdx] = slotBank;
        }

        for (int i = 0; i < osFactor; i++) {
            auto mainSignal = Math::hsum (frameArgs.mainSignal [i]);
            auto restSignal = Math::hsum (frameArgs.restSignal [i]);

            /*if (dcFilterOn) {

            }*/

            pulsarOut.mainSignal [i] [bankIdx] = softClip (mainSignal);
            pulsarOut.restSignal [i] [bankIdx] = softClip (restSignal);
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
        auto osSampleTime = 1.f / (args.sampleRate * oversampleFactor);
        VectorT syncBuffer [MaxOversample];

        for (uint32_t bank = 0, baseChannel = 0; baseChannel < channelCount; bank++, baseChannel += SIMDBankSize) {
            if (args.syncEnabled)
                syncUpsampler [bank].process (syncBuffer, VectorT::load (args.syncVoltages + baseChannel));

            for (int sampleIdx = 0; sampleIdx < oversampleFactor; sampleIdx++) {
                auto emitMask = VectorT::zero ();

                auto deltaPhase = VectorT::load (emissionFrequency + baseChannel) * osSampleTime;
                auto phase = VectorT::load (emissionPhase + baseChannel);

                phase += deltaPhase & triggeredModeMask;
                emitMask |= (phase >= 1.f) & triggeredModeMask;

                if (args.syncEnabled) {
                    auto syncValue = syncBuffer [sampleIdx];
                    auto deltaSync = syncValue - lastSyncValues [bank];
                    auto syncCrossing = -lastSyncValues [bank] / deltaSync;
                    lastSyncValues [bank] = syncValue;

                    auto sync = (syncCrossing > 0.f) & (syncCrossing <= 1.f) & (syncValue >= 0.f);
                    phase = rack::simd::ifelse (sync, (1.f - syncCrossing) * deltaPhase, phase);
                    emitMask |= sync;
                }

                phase -= rack::simd::floor (phase);
                phase.store (emissionPhase + baseChannel);
                auto emitMaskInt = rack::simd::movemask (emitMask);

                auto bankSize = std::min (channelCount - baseChannel, SIMDBankSize);
                for (uint32_t bankIdx = 0; bankIdx < bankSize; bankIdx++) {
                    if (!(emitMaskInt & (1 << bankIdx)))
                        continue;

                    auto channel = baseChannel + bankIdx;
                    auto mask = maskingData [channel].process ();
                    if (mask != PulsarMask::None)
                        emitPulsar (args, channel, mask == PulsarMask::Rest, sampleIdx);
                }
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