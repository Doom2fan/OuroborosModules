/*
 *  OuroborosModules
 *  Copyright (C) 2024 Chronos "phantombeta" Ouroboros
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

#include "Math.hpp"

namespace OuroborosModules::Math {
    void Sort3 (rack::simd::float_4 vecA, rack::simd::float_4 vecB, rack::simd::float_4 vecC,
                rack::simd::float_4& vecMin, rack::simd::float_4& vecMid, rack::simd::float_4& vecMax) {
        auto temp = rack::simd::fmax (vecA, vecC);
        vecA = rack::simd::fmin (vecA, vecC);
        vecC = temp;

        temp = rack::simd::fmax (vecA, vecB);
        vecA = rack::simd::fmin (vecA, vecB);
        vecB = temp;

        temp = rack::simd::fmax (vecB, vecC);
        vecB = rack::simd::fmin (vecB, vecC);
        vecC = temp;

        vecMin = vecA;
        vecMid = vecB;
        vecMax = vecC;
    }
}