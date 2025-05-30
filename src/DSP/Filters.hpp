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

namespace OuroborosModules::DSP {
    template<typename T>
    struct Butterworth6P {
      private:
        rack::dsp::TBiquadFilter<T> biquad [3] {};

      public:
        void setCutoffFreq (float normalizedCutoff) {
            assert (normalizedCutoff > 0 && normalizedCutoff < .5f);

            biquad [0].setParameters (rack::dsp::TBiquadFilter<T>::LOWPASS, normalizedCutoff, .51763809f, 1);
            biquad [1].setParameters (rack::dsp::TBiquadFilter<T>::LOWPASS, normalizedCutoff, .70710678f, 1);
            biquad [2].setParameters (rack::dsp::TBiquadFilter<T>::LOWPASS, normalizedCutoff, 1.9318517f, 1);
        }

        T process (T x) {
            x = biquad [0].process (x);
            x = biquad [1].process (x);
            x = biquad [2].process (x);
            return x;
        }
    };

    template<typename T>
    struct UpsampleFilter {
      private:
        int oversampleFactor = 1;
        Butterworth6P<T> filter = Butterworth6P<T> ();

      public:
        void setOversampleRate (int newOversampleFactor) {
            oversampleFactor = newOversampleFactor;
            filter.setCutoffFreq (1.f / (newOversampleFactor * 4));
        }

        T process (T x) { return filter.process (x); }

        void process (T* outputBuffer, T input) {
            outputBuffer [0] = filter.process (input * oversampleFactor);

            auto zero = T (0);
            for (int i = 1; i < oversampleFactor; ++i)
                outputBuffer [i] = filter.process (zero);
        }
    };

    template<typename T>
    struct DownsampleFilter {
      private:
        int oversampleFactor = 1;
        Butterworth6P<T> filter = Butterworth6P<T> ();

      public:
        void setOversampleRate (int newOversampleFactor) {
            oversampleFactor = newOversampleFactor;
            filter.setCutoffFreq (1.f / (newOversampleFactor * 4));
        }

        T process (T x) { return filter.process (x); }

        T process (T* outputBuffer) {
            for (int i = 0; i < oversampleFactor - 1; ++i)
                filter.process (outputBuffer [i]);
            return filter.process (outputBuffer [oversampleFactor - 1]);
        }
    };
}