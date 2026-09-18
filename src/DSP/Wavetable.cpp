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

#include "Wavetable.hpp"

namespace OuroborosModules::DSP {
    /*
     * Wavetable
     */
    float Wavetable::getSample (uint32_t frameIndex, uint32_t sampleIndex, uint32_t octave) const {
        assert (frameIndex < frameCount);
        assert (sampleIndex < waveLength);
        assert (octave < octaves);

        if (sampleIndex >= waveLength ||
            frameIndex >= frameCount ||
            octave >= octaves
        ) {
            return 0.f;
        }

        auto waveLen = waveLength + WavetableInterpolators::MaxSampleCount;
        return samples [
            octave * frameCount * waveLen + // Z
            frameIndex * waveLen + // Y
            sampleIndex // X
        ];
    }

    void Wavetable::getSamples (
        float* outSamples, uint32_t sampleCount, uint32_t frameIndex, uint32_t sampleIndex, uint32_t octave) const {
        assert (outSamples != nullptr);
        assert (sampleCount > 1 && sampleCount < waveLength);
        assert (frameIndex < frameCount);
        assert (sampleIndex < waveLength);
        assert (octave < octaves);

        if (outSamples == nullptr ||
            sampleCount < 1 || sampleCount >= waveLength ||
            frameIndex >= frameCount ||
            sampleIndex >= waveLength ||
            octave >= octaves
        ) {
            return;
        }

        auto waveLen = waveLength + WavetableInterpolators::MaxSampleCount;
        auto zOffs = octave * frameCount * waveLen;
        auto yOffs = frameIndex * waveLen;
        if (sampleIndex + sampleCount < waveLen) {
            auto initialIdx = zOffs + yOffs + sampleIndex;
            std::copy (samples.begin () + initialIdx, samples.begin () + initialIdx + sampleCount, outSamples);
        } else {
            auto initialIdx = zOffs + yOffs + sampleIndex;
            std::copy_n (samples.begin () + initialIdx, waveLength - initialIdx, outSamples);
            auto remainder = sampleCount - (waveLength - initialIdx);
            std::copy_n (samples.begin (), remainder, outSamples);
        }
    }

    bool Wavetable::setSamples (const float* newSamples, uint32_t waveLength, uint32_t frameCount, uint32_t maxOctaves) {
        using rack::dsp::RealFFT;

        static constexpr auto InterpSamples = WavetableInterpolators::MaxSampleCount;
        static constexpr float minVal = .000001f; // -120 dB

        assert (newSamples != nullptr);
        assert (waveLength > 0);
        assert (waveLength % 32 == 0);
        assert (frameCount > 0);

        if (newSamples == nullptr ||
            waveLength < 1 || (waveLength % 32) != 0 ||
            frameCount < 1
        ) {
            return false;
        }

        // Calculate frame FFTs, max harmonics and octave count
        RealFFT fft (waveLength);
        auto inSpace = new float [waveLength] ();
        std::vector<float*> fftFrames = { };

        uint32_t allMaxHarmonics = 1u;
        for (uint32_t frameIdx = 0; frameIdx < frameCount; frameIdx++) {
            auto frameFFT = new float [2 * waveLength] ();
            fftFrames.emplace_back (frameFFT);

            // Compute the frame's FFT
            for (uint32_t j = 0; j < waveLength; j++)
                inSpace [j] = newSamples [waveLength * frameIdx + j] / waveLength;
            fft.rfft (inSpace, frameFFT);

            // Remove highest harmonic
            frameFFT [1] = 0.f;

            // Find the highest harmonic
            auto maxHarmonic = waveLength >> 1;
            while ((fabs (frameFFT [2 * maxHarmonic]) + abs (frameFFT [2 * maxHarmonic + 1]) < minVal) &&
                   maxHarmonic > 0) {
                maxHarmonic--;
            }

            allMaxHarmonics = std::max (maxHarmonic, allMaxHarmonics);
        }

        // Calculate info
        auto octaves = std::min (static_cast<uint32_t> (std::ceil (std::log2 (allMaxHarmonics))) + 1, maxOctaves);
        auto totalWaveLen = waveLength + InterpSamples;
        auto totalSampleCount = octaves * frameCount * totalWaveLen;

        // Resize arrays
        samples.resize (totalSampleCount);
        samples.shrink_to_fit ();

        // Generate octaves
        auto fftWorkspace = new float [2 * waveLength] ();

        auto samplesData = samples.data ();
        auto octave = 0;
        auto maxHarmonic = allMaxHarmonics;
        while (maxHarmonic) {
            for (uint32_t frameIdx = 0; frameIdx < frameCount; frameIdx++) {
                auto frameFFT = fftFrames [frameIdx];

                // Fill the table with harmonics
                std::fill_n (fftWorkspace, 2 * waveLength, 0.f);
                std::copy_n (frameFFT, 2 * (maxHarmonic + 1), fftWorkspace);

                // Calculate wave frame pos
                auto wavePos = octave * frameCount * totalWaveLen + frameIdx * totalWaveLen;

                // Reverse FFT
                fft.irfft (fftWorkspace, samplesData + wavePos);

                // Copy the interpolation samples
                std::copy_n (samplesData + wavePos, InterpSamples, samplesData + wavePos + waveLength);
            }

            // Octave cut
            maxHarmonic >>= 1;
            octave++;
        }

        delete [] fftWorkspace;
        delete [] inSpace;
        for (auto fftFrame : fftFrames) {
            if (fftFrame != nullptr)
                delete [] fftFrame;
        }

        // Store the wavetable info
        this->waveLength = waveLength;
        this->frameCount = frameCount;
        this->octaves = octaves;
        this->baseHarmonics = allMaxHarmonics;

        return true;
    }
}