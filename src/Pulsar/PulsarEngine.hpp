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

#pragma once

#include "../DSP/Wavetable.hpp"
#include "../DSP/Resamplers.hpp"
#include "../PluginDef.hpp"

namespace OuroborosModules::Modules::Pulsar {
    typedef uint16_t SlotBankType;
    static constexpr uint32_t MaxPulsars = 16;
    static constexpr uint32_t SlotBankSize = sizeof (SlotBankType) * 8;
    static constexpr uint32_t SlotBankCount = (MaxPulsars + SlotBankSize - 1) / SlotBankSize;

    static constexpr uint32_t WavetableLength = 2048; // Must be a power of two
    static constexpr uint32_t WavesCount = 5;
    static constexpr uint32_t WindowsCount = 4;

    static constexpr float MinFreqHz = 20.f;
    static constexpr float MaxFreqHz = 20000.f;

    static constexpr uint8_t DefaultOversampleRate = 4;
    static constexpr uint8_t MaxOversample = 16;

    enum class PulsarMaskingMode {
        Invalid, // Not used for anything, it's here to catch errors
        Burst,
        Stochastic
    };

    enum class PulsarMask {
        None,
        Pulse,
        Rest,
    };

    enum class PulsarFrequencyMode {
        Audio,
        LFO,
        Triggered,
    };

    struct PulsarWavetables;

    struct PulsarProcessArgs {
        float sampleRate = 0.f;
        float sampleTime = 0.f;

        float edgeFactor = 0.f;

        bool syncEnabled = false;
        float syncVoltages [Constants::MaxPolyphony] = { };

        float* mainOut = nullptr;
        float* restOut = nullptr;

        // Internal state
        PulsarWavetables* wavetables = nullptr;
    };

    struct PulsarMaskingData {
      private:
        PulsarMaskingMode mode = PulsarMaskingMode::Invalid;
        union {
            struct {
                // Parameters
                uint32_t burstCount;
                uint32_t restCount;

                // State
                uint32_t curCount;
                bool isRest;
            } burstData;

            struct {
                // Parameters
                float restProbability;
                float skipProbability;
            } stochasticData;
        };

      public:
        PulsarMaskingData () { setBurst (1, 0); }

        json_t* dataToJson () const;
        bool dataFromJson (json_t* rootJ);

        PulsarMaskingMode getMode () { return mode; }
        void setBurst (uint32_t burstCount, uint32_t restCount);
        void setStochastic (float restProb, float noneProb);

        PulsarMask process ();
    };

    struct PulsarParameters {
        // Parameters
        bool isRest = false;

        float frequency = 0.f;
        float cluster = 0.f;

        float shapeIndex = 0.f;
        float shaperAmount = 0.f;

        float windowIndex = 0.f;
        float windowSkew = 0.f;
        float windowAmount = 0.f;

        // State
        float wavePhase = 0.f;
        float windowPhase = 0.f;
        float edgePhase = -1.f;

        json_t* dataToJson () const;
        bool dataFromJson (json_t* rootJ);
    };

    struct PulsarDataStore {
        SlotBankType slotUsed [SlotBankCount] = { };

        // Parameters
        alignas (16) float isRest [MaxPulsars] = { };
        float frequency [MaxPulsars] = { };
        float cluster [MaxPulsars] = { };

        uint32_t waveIndex [MaxPulsars] = { };
        float waveIndexFrac [MaxPulsars] = { };
        float shaperAmount [MaxPulsars] = { };

        uint32_t windowIndex [MaxPulsars] = { };
        float windowIndexFrac [MaxPulsars] = { };
        float windowSkew [MaxPulsars] = { };
        float windowAmount [MaxPulsars] = { };

        // State
        float wavePhase [MaxPulsars] = { };
        float windowPhase [MaxPulsars] = { };
        float edgePhase [MaxPulsars] = { };

        uint32_t wave0Octave [MaxPulsars] = { };
        uint32_t wave1Octave [MaxPulsars] = { };
        uint32_t window0Octave [MaxPulsars] = { };
        uint32_t window1Octave [MaxPulsars] = { };

        json_t* dataToJson () const;
        bool dataFromJson (json_t* rootJ);

        void setSlot (uint32_t index, bool used);
        void copyToSlot (uint32_t index, const PulsarParameters& params);
        void slotToParams (uint32_t index, PulsarParameters& params) const;
        PulsarParameters slotToParams (uint32_t index) const {
            PulsarParameters params {};
            slotToParams (index, params);
            return params;
        }
    };

    struct NoiseBuffer {
        static constexpr uint32_t BufferSize = 16; // Must be a power of two
        static constexpr uint32_t BufferSizeMask = (BufferSize - 1);

        float buffer [BufferSize];
        uint32_t writeIdx = 0;
        uint32_t readIdx = 0;

    public:
        NoiseBuffer ();

        void update ();
        float read ();
        void readCount (float* outBuffer, uint32_t count);
    };

    struct PulsarEngine {
        using VectorT = rack::simd::float_4;

        static constexpr uint32_t SIMDBankSize = VectorT::size;
        static constexpr uint32_t SIMDBankCount = static_cast<uint32_t> (static_cast<float> (Constants::MaxPolyphony) / SIMDBankSize + .5f);

        struct PulsarFrameArgs {
            // All
            float osSampleRate = 0.f;
            float osSampleTime = 0.f;

            float edgeFrequency = 0.f;

            PulsarDataStore* pulsars = nullptr;
            PulsarWavetables* wavetables = nullptr;

            VectorT mainSignal [MaxOversample] = { };
            VectorT restSignal [MaxOversample] = { };

            // Per-quad
            uint32_t baseIndex = 0;
            uint32_t slotMask = 0;
        };

        struct PulsarOutput {
            VectorT mainSignal [MaxOversample] = { };
            VectorT restSignal [MaxOversample] = { };
        };

      private:
        // Parameters
        PulsarParameters parameters [Constants::MaxPolyphony];

        float emissionFrequency [Constants::MaxPolyphony] = { };

        bool overlapMode = false;
        bool triggeredMode = false;

        uint32_t channelCount = 1;
        bool outputActiveMain = false;
        bool outputActiveRest = false;

        // State
        float curSampleRate = 0.f;
        PulsarDataStore pulsars [Constants::MaxPolyphony];
        PulsarMaskingData maskingData [Constants::MaxPolyphony];

        VectorT lastSyncValues [SIMDBankCount] = { };
        float emissionPhase [Constants::MaxPolyphony] = { };

        std::shared_ptr<PulsarWavetables> wavetables = nullptr;
        NoiseBuffer noiseBuffer;

        int oversampleFactor = 0;
        DSP::OptimizedHalfBandDecimator<VectorT> decimatorMain [SIMDBankCount];
        DSP::OptimizedHalfBandDecimator<VectorT> decimatorRest [SIMDBankCount];

      public:
        PulsarEngine ();

        json_t* channelToJson (uint32_t channel) const;
        bool channelFromJson (json_t* rootJ, uint32_t channel);

        json_t* dataToJson () const;
        bool dataFromJson (json_t* rootJ);

        void setOversampling (uint8_t factor, bool force);

        void setOverlapMode (bool overlap);
        void setTriggeredMode (bool enable);

        void setEmissionFrequency (uint32_t channel, float freq);
        void setParams (uint32_t channel, const PulsarParameters& params);
        void setMaskingBurst (uint32_t channel, uint32_t burstCount, uint32_t restCount);
        void setMaskingStochastic (uint32_t channel, float restProbability, float noneProbability);

        void setChannelCount (uint32_t count);
        void setActiveOutputs (bool main, bool rest);

        void emitPulsar (uint32_t channel, const PulsarParameters& params, bool isRest, float phase);

        inline void processPulsarQuad (PulsarFrameArgs& args);
        void processPulsars (PulsarProcessArgs& args, PulsarOutput& pulsarOut, uint32_t channel, uint32_t bankIdx);
        void processPulsars (PulsarProcessArgs& args);
        void processEmission (PulsarProcessArgs& args);
        void process (PulsarProcessArgs& args);

        void setPulsarOctave (uint32_t channel, uint32_t slot);
        void updatePulsarOctaves (float sampleRate);
        void onSampleRateChange (float newSampleRate);
    };
}