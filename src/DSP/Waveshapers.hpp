/*
 *  OuroborosModules
 *  Copyright (C) 2024-2025 Chronos "phantombeta" Ouroboros
 *  Copyright (C) ChowDSP
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

namespace OuroborosModules::DSP::Waveshapers {
    /** Output range: -1, 1 */
    template<typename T>
    T softClip (T sample) {
        return sample / rack::simd::sqrt (1 + sample*sample);
    }

    /** Output range: -1, 1 */
    template<typename T>
    T softClipApprox (T sample) {
        return sample * Math::rsqrt_nr1 (1 + sample*sample);
    }

    /** Output range: -1, 1 */
    template<typename T>
    T reinhard (T sample) {
        return sample / (T (1) + rack::simd::abs (sample));
    }

    /** Output range: 0, 1 */
    template<typename T>
    T logistic (T sample) {
        // This correction factor (1 / ln (2)) makes 2^-x match e^-x
        static constexpr T correctionFactor = T (1.44269504088896340735992468100189213742664595415299);
        return T (1) / (T (1) + rack::dsp::exp2_taylor5 (-sample * correctionFactor));
    }
}