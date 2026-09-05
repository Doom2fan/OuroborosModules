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

#include "../PluginDef.hpp"
#include "../DSP/DelayLine.hpp"
#include "../DSP/ChowDSP_IIR.hpp"
#include "../DSP/SchroederAllpass.hpp"

namespace OuroborosModules::Modules::Thrum {
    static constexpr float MinFreqHz = 20.f;
    static constexpr float MaxFreqHz = 5000.f;
    static constexpr float MinAPFreqHz = 20.f;
    static constexpr float MaxAPFreqHz = 20000.f;
    static constexpr float MinSnareFreqHz = 20.f;
    static constexpr float MaxSnareFreqHz = 20000.f;

    static constexpr float DefaultFreqHz = 120.f;
    static constexpr float DefaultAPFreqHz = 1000.f;
    static constexpr float DefaultSnareFreqHz = 3000.f;

    enum class SnareFilterType {
        Bandpass = 0,
        Lowpass = 1,
    };

    struct DrumCore {
      private:
        // Params
        float baseFrequency = DefaultFreqHz;

        float exciter_ResetOnHit = true;
        float exciter_SineLevel = 1.f;
        float exciter_NoiseLevel = 1.f;

        float exciter_NoiseBleedLevel = 1.f;

        float feedbackLevel = .985f;
        float dampingFreq = 1000.f;

        float allpass_Freq = DefaultAPFreqHz;
        float allpass_Gain = .5f;

        float pitchbend_Decay = 80.f / 1000.f;
        float pitchbend_Depth = 1.f;

        float snare_Level = .125f;
        float snare_FilterFreq = DefaultSnareFreqHz;
        SnareFilterType snare_FilterType = SnareFilterType::Bandpass;

        float curVelocity = 1.f;
        float sqrtVelocity = 1.f;

        // Misc state
        float curSampleRate = 48000;

        // Exciter
        float exciter_Phase = 0.f;
        float exciter_Velocity = 0.f;

        rack::random::Xoroshiro128Plus exciter_RNG;
        float exciter_NoiseFilterFreq = 0.f;
        DSP::TBiquadFilter<float> exciter_Filter;

        // Loop
        float delayTimeFrac = 0.f;
        DSP::DelayLine<float> delayLine;

        DSP::SchroederAllpass<float> allpass;

        rack::dsp::TRCFilter<float> loopFilter;
        rack::dsp::TRCFilter<float> loopHighpass;

        // Pitchbend
        float pitchbend_Phase;

        // Snares
        rack::dsp::TPeakFilter<float> snare_PeakFilter;
        DSP::TBiquadFilter<float> snare_Filter;

        void setDelayTime (float delayTime);

        void updateExciterNoiseFilter ();
        void updateSnareFilter ();

      public:
        void setFrequency (float freq);

        void setExciterParams (float sineLevel, float noiseLevel);
        void setExciterBleedParams (float noiseBleedLevel);

        void setFeedback (float feedback, float damping);
        void setAllpassParams (float freq, float gain);

        void setPitchbendParams (float decay, float depth);
        void setSnareParams (float level, float filterFreq, SnareFilterType filterType, float fallTime);

        void hit (float velocity, bool reset);

        float process (const rack::Module::ProcessArgs& args);
        void onSampleRateChange (float newSampleRate);
    };
}