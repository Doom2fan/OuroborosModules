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
#include "DelayLineInterpolators.hpp"

#include <algorithm>
#include <cstdint>

namespace OuroborosModules::DSP {
    template<typename TSampleType>
    struct DelayLine {
      private:
        int32_t maxSamples = 0;
        TSampleType* samples = nullptr;

        int32_t delayTime = 0;
        int32_t posIndex = 0;

        int32_t getBufferLength () { return maxSamples + DelayLineInterpolators::MaxSampleCount; }

      public:
        DelayLine () {
            setMaxSamples (256);
            delayTime = maxSamples;
        }

        DelayLine (int32_t maxSamples) {
            setMaxSamples (maxSamples);
            delayTime = maxSamples;
        }

        ~DelayLine () {
            if (samples != nullptr)
                delete [] samples;
        }

        int32_t getMaxSamples () const { return maxSamples; }

        void setMaxSamples (int32_t newMaxSamples) {
            assert (newMaxSamples > 0);
            if (newMaxSamples < 1)
                return;

            if (samples != nullptr)
                delete [] samples;

            maxSamples = newMaxSamples;
            samples = new TSampleType [getBufferLength ()];

            delayTime = std::clamp (delayTime, 1, maxSamples);
            posIndex = 0;
            resetFull ();

            std::fill_n (samples, getBufferLength (), TSampleType (0));
        }

        void setDelayTime (int32_t newDelayTime) {
            assert (newDelayTime > 0);
            assert (newDelayTime <= maxSamples);

            delayTime = std::clamp (newDelayTime, 1, maxSamples);
        }

        void resetFull () {
            std::fill_n (samples, getBufferLength (), TSampleType (0));
            posIndex = 0;
        }

        void reset () {
            auto bufferLen = getBufferLength ();
            auto delaySamples = std::clamp (delayTime + DelayLineInterpolators::MaxSampleCount, 0, bufferLen);
            if (posIndex - delaySamples >= 0)
                std::fill_n (samples + (posIndex + 1 - delaySamples), delaySamples, TSampleType (0));
            else {
                std::fill_n (samples, posIndex + 1, TSampleType (0));
                auto remainder = delaySamples - (posIndex + 1);
                std::fill_n (samples + (bufferLen - remainder), remainder, TSampleType (0));
            }
        }

        TSampleType getSample (int32_t index) {
            assert (index >= 0);
            assert (index <= delayTime);

            auto bufferLen = getBufferLength ();

            index = (posIndex - std::clamp (index, 0, delayTime)) % bufferLen;
            index = index < 0 ? bufferLen + index : index;

            return samples [index];
        }

        template<typename TInterpolator>
        TSampleType getSample (float index) {
            assert (index >= 0);
            assert (index <= delayTime);

            auto bufferLen = getBufferLength ();

            index = std::clamp (index, 0.f, static_cast<float> (delayTime));

            auto delayInt = static_cast<int32_t> (std::floor (index));
            auto delayFrac = index - delayInt;

            delayInt = (posIndex - delayInt) % bufferLen;
            delayInt = delayInt < 0 ? bufferLen + delayInt : delayInt;

            return TInterpolator::interpolate (samples, delayInt, 1.f - delayFrac, bufferLen);
        }

        template<typename TInterpolator>
        TSampleType getSampleFrac (float delayFrac) {
            assert (delayFrac >= 0.f && delayFrac <= 1.f);
            delayFrac = std::clamp (delayFrac, 0.f, 1.f);

            auto bufferLen = getBufferLength ();

            auto index = posIndex - delayTime;
            index = index < 0 ? bufferLen + index : index;

            return TInterpolator::interpolate (samples, index, 1.f - delayFrac, bufferLen);
        }

        void pushSample (TSampleType newSample) {
            if (++posIndex >= getBufferLength ())
                posIndex = 0;

            samples [posIndex] = newSample;
        }

        TSampleType process (TSampleType newSample) {
            auto ret = samples [posIndex];
            pushSample (newSample);

            return ret;
        }

        template<typename TInterpolator>
        TSampleType process (TSampleType newSample, float delayFrac) {
            assert (delayFrac >= 0.f && delayFrac <= 1.f);

            auto ret = getSampleFrac<TInterpolator> (delayFrac);
            pushSample (newSample);

            return ret;
        }
    };
}