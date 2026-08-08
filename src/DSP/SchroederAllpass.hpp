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

#include "../Math.hpp"
#include "../PluginDef.hpp"
#include "DelayLine.hpp"

namespace OuroborosModules::DSP {
    template<typename T>
    struct SchroederAllpass {
      private:
        int32_t delayTimeInt;
        float delayTimeFrac;
        float gain = 0.f;
        float tGain = 1.f;

        int32_t delayPos = 0;
        DelayLine<T> delayLine;

      public:
        SchroederAllpass () { setGain (1.f); }

        SchroederAllpass (int32_t maxDelay) : delayLine (maxDelay) { setGain (1.f); }

        void setMaxDelay (int32_t newMaxDelay) {
            assert (newMaxDelay > 0);

            delayLine.setMaxSamples (newMaxDelay);
        }

        float getGain () { return gain; }
        void setGain (float newGain) {
            assert (newGain >= -1.f);
            assert (newGain <=  1.f);

            newGain = std::clamp (newGain, -1.f, 1.f);
            if (newGain == gain)
                return;

            gain = newGain;
            tGain = std::sqrt (1.f - gain*gain);
        }

        float getDelayTime () { return delayTimeInt + delayTimeFrac; }
        void setDelayTime (float newDelayTime) {
            assert (newDelayTime > 0);
            assert (newDelayTime <= delayLine.getMaxSamples ());

            auto maxSamples = static_cast<float> (delayLine.getMaxSamples ());
            newDelayTime = std::clamp (newDelayTime, 1.f, maxSamples);

            delayTimeInt = static_cast<int32_t> (std::floor (newDelayTime));
            delayTimeFrac = newDelayTime - delayTimeInt;

            delayLine.setDelayTime (delayTimeInt);
        }

        void resetFull () { delayLine.resetFull (); }
        void reset () { delayLine.reset (); }

        T process (T x) {
            using Math::fpClean;

            x = fpClean (x);

            auto delay = delayLine.template getSampleFrac<DSP::DelayLineInterpolators::Lagrange<5>> (delayTimeFrac);
            auto node1 = fpClean (x * tGain - delay * gain);
            auto node2 = fpClean (delay * tGain + x * gain);
            delayLine.pushSample (node1);

            return node2;
        }
    };
}