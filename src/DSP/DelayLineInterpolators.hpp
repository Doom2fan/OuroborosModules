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

#include <cstdint>

namespace OuroborosModules::DSP::DelayLineInterpolators {
    // The highest number of samples needed by any interpolator
    static constexpr int32_t MaxSampleCount = 32;

    /* Interface:
    struct IInterpolator {
        // buffer: Non-null
        // delayInt: 0 <= delayIndex + delayFrac < delayLength
        // delayFrac: 0 <= delayFrac < 1
        // delayLength: 0 < delayLength < buffer length
        TSampleType interpolate (const TSampleType* buffer, int32_t delayInt, float delayFrac, int32_t delayLength);
    };
    */

    /*
     * No interpolation
     */
    struct None {
        template<typename TSampleType>
        inline static TSampleType interpolate (const TSampleType* buffer, int32_t delayInt, float delayFrac, int32_t delayLength) {
            return buffer [delayInt];
        }
    };

    /*
     * Linear interpolation.
     * Fast, but effectively lowpass filtering.
     */
    struct Linear {
        template<typename TSampleType>
        inline static TSampleType interpolate (const TSampleType* buffer, int32_t delayInt, float delayFrac, int32_t delayLength) {
            auto z0 = buffer [delayInt > 0 ? delayInt - 1 : delayLength - 1];
            auto z1 = buffer [delayInt];

            return z0 + TSampleType (delayFrac) * (z1 - z0);
        }
    };

    /*
     * Lagrange interpolation.
     * Slower than linear, but less lowpass filtering.
     */
    template<int ORDER>
    struct Lagrange {
        inline static constexpr float lagrangeDenom (int n) {
            float accum = 1.f;
            for (int k = 0; k <= ORDER; k++) {
                if (k != n)
                    accum *= (n - k);
            }

            return accum;
        }

        template<typename TSampleType>
        inline static TSampleType interpolate (const TSampleType* buffer, int32_t delayInt, float delayFrac, int32_t delayLength) {
            delayInt -= ORDER;
            delayFrac += static_cast<int> (std::floor (ORDER / 2.f));
            if (delayInt < 0)
                delayInt = delayLength + delayInt;

            TSampleType accum = TSampleType (0);
            for (int i = ORDER; i >= 0; i--) {
                auto num = 1.f;
                for (int j = 1; j <= ORDER; j++) {
                    if (j != i)
                        num *= delayFrac - j;
                }

                if (i == 0)
                    accum *= delayFrac;

                accum += buffer [(delayInt + i) % delayLength] * (num / lagrangeDenom (i));
            }

            return accum;
        }
    };
}