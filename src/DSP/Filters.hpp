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

#pragma once

#include "../PluginDef.hpp"

#include "ChowDSP_IIR.hpp"

namespace OuroborosModules::DSP {
    template<typename T>
    struct Butterworth6P {
      private:
        TBiquadFilter<T> biquad [3] {};

      public:
        void setCutoffFreq (float normalizedCutoff) {
            assert (normalizedCutoff > 0 && normalizedCutoff < .5f);

            biquad [0].setParameters (TBiquadFilter<T>::LOWPASS, normalizedCutoff, .51763809f, 1);
            biquad [1].setParameters (TBiquadFilter<T>::LOWPASS, normalizedCutoff, .70710678f, 1);
            biquad [2].setParameters (TBiquadFilter<T>::LOWPASS, normalizedCutoff, 1.9318517f, 1);
        }

        T process (T x) {
            x = biquad [0].process (x);
            x = biquad [1].process (x);
            x = biquad [2].process (x);
            return x;
        }
    };

    template<typename T>
    struct DCBlocker {
      private:
        // Param
        float r = 0.f;

        // State
        T x1 = 0.f, y1 = 0.f;

      public:
        void setCutoffFreq (float cutoffFreq, float sampleRate) {
            assert (cutoffFreq > 0.f && cutoffFreq < sampleRate / 2.f);
            assert (sampleRate > 0.f);

            setCoefficient (std::expf (-2.f * M_PI * cutoffFreq / sampleRate));
        }

        void setCoefficient (float paramR) {
            assert (paramR > 0.f && paramR < 1.f);
            r = paramR;
        }

        T process (T x) {
            auto y = x - x1 + r * y1;
            x1 = x;
            y1 = y;

            return y;
        }

        void reset () {
            x1 = y1 = 0.f;
        }
    };
}