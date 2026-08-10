/*
 *  OuroborosModules
 *  Copyright (C) 2024 Chronos "phantombeta" Ouroboros
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

#include "DrumCore.hpp"

namespace OuroborosModules::Modules::Thrum {
    void DrumCore::onSampleRateChange (float newSampleRate) {
        curSampleRate = newSampleRate;

        // Loop.
        auto maxDelaySamples = static_cast<int32_t> (std::ceil (newSampleRate / 20.f + 5.f));
        delayLine.setMaxSamples (maxDelaySamples);
        delayLine.resetFull ();
        allpass.setMaxDelay (maxDelaySamples);
        allpass.resetFull ();

        loopHighpass.setCutoffFreq (20.f / newSampleRate);
        loopFilter.setCutoffFreq (dampingFreq / curSampleRate);

        // Exciter.
        updateExciterNoiseFilter ();

        // Snares.
        updateSnareFilter ();
    }

    void DrumCore::updateExciterNoiseFilter () {
        exciter_Filter.setParameters (DSP::TBiquadFilter<float>::LOWPASS, exciter_NoiseFilterFreq / curSampleRate, .4f, 1.f);
    }

    void DrumCore::updateSnareFilter () {
        auto filterType = DSP::TBiquadFilter<float>::LOWPASS;
        switch (snare_FilterType) {
            default:
            case SnareFilterType::Lowpass: break;
            case SnareFilterType::Bandpass: filterType = DSP::TBiquadFilter<float>::BANDPASS; break;
        }

        snare_Filter.setParameters (filterType, snare_FilterFreq / curSampleRate, 1.f, 1.f);
    }

    void DrumCore::setFrequency (float frequency) {
        baseFrequency = frequency;
    }

    void DrumCore::setExciterParams (float sineLevel, float noiseLevel) {
        exciter_SineLevel = sineLevel;
        exciter_NoiseLevel = noiseLevel;
    }

    void DrumCore::setExciterBleedParams (float noiseBleedLevel) {
        exciter_NoiseBleedLevel = noiseBleedLevel;
    }

    void DrumCore::setFeedback (float feedback, float damping) {
        feedbackLevel = feedback;
        dampingFreq = damping;

        loopFilter.setCutoffFreq (dampingFreq / curSampleRate);
    }

    void DrumCore::setAllpassParams (float freq, float gain) {
        allpass_Freq = freq;
        allpass_Gain = gain;
    }

    void DrumCore::setPitchbendParams (float decay, float depth) {
        pitchbend_Decay = decay;
        pitchbend_Depth = depth;
    }

    void DrumCore::setSnareParams (float level, float filterFreq, SnareFilterType filterType, float fallTime) {
        snare_Level = level;
        snare_FilterFreq = filterFreq;
        snare_FilterType = filterType;
        snare_PeakFilter.setTau (fallTime);

        updateSnareFilter ();
    }

    void DrumCore::hit (float velocity, bool reset) {
        // Params.
        curVelocity = velocity;
        sqrtVelocity = rack::simd::sqrt (curVelocity);

        // Exciter.
        exciter_ResetOnHit = reset;

        exciter_Velocity = rack::dsp::dbToAmplitude (-24.f * (1.f - velocity));
        exciter_Phase = 1.f;

        exciter_RNG.seed (211651984, 61321964984);

        exciter_NoiseFilterFreq = baseFrequency * (rack::dsp::exp2_taylor5 (curVelocity * 3.f) / 3.f);
        exciter_NoiseFilterFreq = std::clamp (exciter_NoiseFilterFreq, 20.f, curSampleRate / 2.f);
        updateExciterNoiseFilter ();

        // Pitch bend.
        pitchbend_Phase = 1.f;

        // Loop.
        if (reset) {
            allpass.reset ();
            loopFilter.reset ();
        }
    }

    void DrumCore::setDelayTime (float newDelayTime) {
        assert (newDelayTime > 0);
        assert (newDelayTime <= delayLine.getMaxSamples ());

        auto maxSamples = static_cast<float> (delayLine.getMaxSamples ());
        newDelayTime = std::clamp (newDelayTime, 0.f, maxSamples);

        delayTimeFrac = newDelayTime - std::floor (newDelayTime);
        delayLine.setDelayTime (static_cast<int32_t> (std::floor (newDelayTime)));
    }

    float getRandom (rack::random::Xoroshiro128Plus& rng) {
        return (rng () >> 32) * 2.32830629e-10f;
    }

    float getNoise (rack::random::Xoroshiro128Plus& rng) {
        return getRandom (rng) * 2 - 1;
    }

    inline float sin_2pi_9 (float x) {
        // Shift argument to [-1, 1]
        x = 2 * x - 1;
        auto x2 = x * x;
        return x * (x2 - 1) * (3.1415211942925003f + x2 * (-2.0247734792732333 + x2 * (.51749274223813413f + x2 * -.063691093590695858f)));
    }

    float DrumCore::process (const rack::Module::ProcessArgs& args) {
        using Math::fpClean;

        // Calculate the pitchbend envelope and advance the phase.
        auto pitchBend = 0.f;
        if (pitchbend_Phase > 0.f) {
            pitchBend = rack::simd::pow (pitchbend_Phase, 10) * pitchbend_Depth;
            pitchbend_Phase = std::max (pitchbend_Phase - args.sampleTime / pitchbend_Decay, 0.f);
        }
        auto pitchBendSqr = pitchBend * pitchBend;

        // Calculate the pitch bend frequency and set the delay line's frequency.
        auto freqBend = baseFrequency * pitchBend * (2 + curVelocity);
        setDelayTime (args.sampleRate / (baseFrequency + freqBend));

        // Get the delayed signal.
        auto signal = delayLine.getSampleFrac<DSP::DelayLineInterpolators::Lagrange<5>> (delayTimeFrac);

        // Feedback.
        signal *= feedbackLevel;

        // Loop highpass. (for DC blocking)
        loopHighpass.process (signal);
        signal = loopHighpass.highpass ();

        // Handle the exciter.
        auto exciterBleed = 0.f;
        if (exciter_Phase > 0.f) {
            // Zero out the delayed signal if Reset On Hit is active.
            signal = exciter_ResetOnHit ? 0.f : signal;

            // Noise exciter. (Scaled by 4 to better match mymembrane~)
            auto exciterRandom = getNoise (exciter_RNG);
            exciterRandom = exciter_Filter.process (exciterRandom) * 4;
            exciterRandom *= exciter_Velocity;
            exciterRandom *= std::min (1.f, (1.f - std::abs (2.f * exciter_Phase - 1.f)) * 10.f); // Shaping

            // Noise exciter bleed.
            exciterBleed += exciterRandom * 3 * exciter_NoiseBleedLevel;

            // Noise exciter level param.
            exciterRandom *= exciter_NoiseLevel;

            // Sine exciter, sine level param, velocity.
            auto exciterSine = sin_2pi_9 (1.f - exciter_Phase) * exciter_SineLevel * exciter_Velocity;

            signal += exciterSine + exciterRandom;
            exciter_Phase = std::max (fpClean (exciter_Phase - args.sampleTime * baseFrequency), 0.f);
        }

        // Allpass.
        auto apBend = allpass_Freq * pitchBendSqr * (1 + curVelocity);
        allpass.setGain (allpass_Gain);
        allpass.setDelayTime (args.sampleRate / (allpass_Freq + apBend));
        signal = allpass.process (fpClean (signal));

        // Loop filter.
        loopFilter.process (signal);
        signal = loopFilter.lowpass ();

        // Pitchbend distortion.
        signal *= 1.f + (pitchBendSqr * sqrtVelocity * .7f);

        // Feedback.
        delayLine.pushSample (fpClean (signal));

        // Apply the exciter bleed and ensure the signal is "clean".
        signal = fpClean (signal + exciterBleed);

        // Snares.
        auto snareEnv = snare_PeakFilter.process (args.sampleTime, std::abs (signal));
        auto snareNoise = snare_Filter.process (rack::random::uniform () * 2 - 1);
        signal += snareNoise * snareEnv * snare_Level;

        return signal;
    }
}