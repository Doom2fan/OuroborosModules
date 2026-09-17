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

#include "PluginDef.hpp"

namespace OuroborosModules::Math {
    static constexpr float Sqrt2 = 1.41421356237309504880168872420969807856967187537695f;
    static constexpr float OneOverSqrt2 = 0.70710678118654752440084436210484903928483593768847f;

    void Sort3 (rack::simd::float_4 vecA, rack::simd::float_4 vecB, rack::simd::float_4 vecC,
                rack::simd::float_4& vecMin, rack::simd::float_4& vecMid, rack::simd::float_4& vecMax);

    template<typename T>
    inline T lerp (T a, T b, T t) {
        return a + t * (b - a);
    }

    /** Rescales `x` from the range `[0, 1]` to `[min, max]` */
    inline float rescale1 (float x, float min, float max) {
        return min + x * (max - min);
    }

    using rack::simd::rcp;
    inline float rcp (const float x) {
        return _mm_cvtss_f32 (_mm_rcp_ss (_mm_set_ss (x)));
    }

    using rack::simd::rsqrt;
    inline float rsqrt (const float x) {
        return _mm_cvtss_f32 (_mm_rsqrt_ss (_mm_set_ss (x)));
    }

    template<typename T>
    inline T rcp_nr1 (const T x) {
        auto y = rcp (x);
        return y * (T (2.) - x * y);
    }

    template<typename T>
    inline T rsqrt_nr1 (const T x) {
        auto y = rsqrt (x);
        return y * (T (3.) - x * y * y) * T (.5);
    }

    inline float hsum (rack::simd::float_4 v) {
        __m128 shuf = _mm_movehdup_ps (v.v);      // broadcast elements 3,1 to 2,0
        __m128 sums = _mm_add_ps (v.v, shuf);
        shuf        = _mm_movehl_ps (shuf, sums); // high half -> low half
        sums        = _mm_add_ss (sums, shuf);
        return        _mm_cvtss_f32 (sums);
    }

    inline bool isNan (const float x) { return std::isnan (x); }
    inline rack::simd::float_4 isNan (const rack::simd::float_4 x) { return x != x; }

    inline bool isInfinity (const float x) { return std::isinf (x); }
    inline rack::simd::float_4 isInfinity (const rack::simd::float_4 x) {
        using rack::simd::float_4;
        using rack::simd::int32_4;

        const auto infMask = float_4::cast (int32_4 (0x7F800000)); // +Inf bit pattern
        return rack::simd::abs (x) == infMask;
    }

    inline float fpClean (const float x) { return std::isfinite (x) ? x : 0; }
    inline rack::simd::float_4 fpClean (const rack::simd::float_4 x) { return x & ~(isNan (x) | isInfinity (x)); }
}