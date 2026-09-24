/*
 *  OuroborosModules
 *  Copyright (C) 2026 Chronos "phantombeta" Ouroboros
 *  Copyright (C) 2016-2026 VCV
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

#include <simd/Vector.hpp>
#include <simd/functions.hpp>

namespace rack::simd {
    /* Functions based on instructions */
    /** `~a & b` */
    inline int32_4 andnot (int32_4 a, int32_4 b) {
        return int32_4 (_mm_andnot_si128 (a.v, b.v));
    }

    /* Nonstandard convenience functions */
    /** Given a mask, returns a if mask is 0xffffffff per element, b if mask is 0x00000000 */
    inline int32_4 ifelse (int32_4 mask, int32_4 a, int32_4 b) { return (a & mask) | andnot (mask, b); }

    /* Standard math functions from std:: */
    inline int32_4 max (int32_4 x, int32_4 b) { return int32_4 (_mm_max_epi32 (x.v, b.v)); }
    inline int32_4 min (int32_4 x, int32_4 b) { return int32_4 (_mm_min_epi32 (x.v, b.v)); }

    inline int32_4 abs (int32_4 a) {
        // Sign bit
        int32_4 mask = ~0x80000000;
        return a & mask;
    }

    /* From math.hpp */
    inline int32_4 clamp (int32_4 x, int32_4 a = 0, int32_4 b = 1) { return min (max (x, a), b); }

    inline int32_4 sgn (int32_4 x) {
        int32_4 signbit = x & -0;
        int32_4 nonzero = (x != 0.f);
        return signbit | (nonzero & 1.f);
    }
}