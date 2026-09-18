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
#include "WavetableInterpolators.hpp"

#include <vector>

namespace OuroborosModules::DSP {
    struct Wavetable {
      private:
        uint32_t waveLength;
        uint32_t frameCount;
        uint32_t octaves;
        uint32_t baseHarmonics;

        std::vector<float> samples = { };

      public:
        Wavetable () {
            float newSamples [32] = { 0.f };
            setSamples (newSamples, 32, 1);
        }

        template<typename T>
        T calculateOctave (T phaseIncrement) const {
            return rack::simd::clamp (std::log2 (2.f * baseHarmonics * phaseIncrement), T (0), T (octaves - 1));
        }

        template<typename T>
        T calculateOctave (float sampleRate, T freq) const {
            return rack::simd::clamp (rack::simd::log2 (2.f * baseHarmonics * freq / sampleRate), T (0), T (octaves - 1));
        }

        uint32_t wrapIndex (uint32_t index) const {
            return index & (waveLength - 1);
        }

        uint32_t getWaveLength () const { return waveLength; }
        uint32_t getFrameCount () const { return frameCount; }
        uint32_t getOctaves () const { return octaves; }

        float getSample (uint32_t frameIndex, uint32_t sampleIndex, uint32_t octave) const;
        void getSamples (float* outSamples, uint32_t sampleCount, uint32_t frameIndex, uint32_t sampleIndex, uint32_t octave) const;

        template<typename TInterpolator>
        float getSample (uint32_t frameIndex, float sampleIndex, uint32_t octave) const {
            assert (frameIndex < frameCount);
            assert (sampleIndex >= 0 && sampleIndex < waveLength);
            assert (octave < octaves);

            if (sampleIndex < 0 || sampleIndex >= waveLength ||
                frameIndex >= frameCount ||
                octave >= octaves
            ) {
                return 0.f;
            }

            auto sampleIndexInt = static_cast<uint32_t> (sampleIndex);
            auto sampleFrac = sampleIndex - sampleIndexInt;
            auto waveLen = waveLength + WavetableInterpolators::MaxSampleCount;
            auto waveOffs = octave * frameCount * waveLen +
                            frameIndex * waveLen;

            return TInterpolator::interpolate (samples.data () + waveOffs, sampleIndexInt, sampleFrac, waveLength);
        }

        template<typename TInterpolator>
        float getSampleFrac (uint32_t frameIndex, uint32_t sampleIndex, float sampleFrac, uint32_t octave) const {
            assert (frameIndex < frameCount);
            assert (sampleIndex < waveLength);
            assert (sampleFrac >= 0 && sampleFrac < 1);
            assert (octave < octaves);

            if (sampleIndex >= waveLength ||
                frameIndex >= frameCount ||
                octave >= octaves
            ) {
                return 0.f;
            }

            auto waveLen = waveLength + WavetableInterpolators::MaxSampleCount;
            auto waveOffs = octave * frameCount * waveLen +
                            frameIndex * waveLen;

            return TInterpolator::interpolate (samples.data () + waveOffs, sampleIndex, sampleFrac, waveLength);
        }

        /** Sets the samples for the wavetable, and generates bandwidth-limited octave mipmaps, if necessary.
         * newSamples: The samples to assign to the wavetable. Not stored.
         * waveLength: The length of the wave in samples. Must be non-zero and a multiple of 32.
         * frameCount: The number of frames in the wavetable. Must be greater than 0.
         * maxOctaves: The maximum number of octaves the wavetable may have. Must be greater than 0.
         */
        bool setSamples (const float* newSamples, uint32_t waveLength, uint32_t frameCount, uint32_t maxOctaves = std::numeric_limits<uint32_t>::max ());
    };
}