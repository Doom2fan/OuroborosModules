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

namespace OuroborosModules::Math {
    template<int TABLE_SIZE = 64>
    struct BezierCurve {
      private:
        rack::math::Vec p0;
        rack::math::Vec p1;
        rack::math::Vec p2;

        float arcLength;
        float tArr [TABLE_SIZE + 1];
        float lenArr [TABLE_SIZE + 1];

      public:
        BezierCurve (rack::math::Vec p0, rack::math::Vec p1, rack::math::Vec p2) : p0 (p0), p1 (p1), p2 (p2) {
            auto prevPoint = p0;
            auto totalLen = 0.f;

            tArr [0] = 0.f;
            lenArr [0] = 0.f;

            for (int i = 1; i <= TABLE_SIZE; i++) {
                float t = i * (1.f / TABLE_SIZE);

                auto curPoint = evaluate (t);
                totalLen += (curPoint - prevPoint).norm ();

                tArr [i] = t;
                lenArr [i] = totalLen;

                prevPoint = curPoint;
            }

            arcLength = totalLen;
        }

        float getArcLength () { return arcLength; }

        rack::math::Vec evaluate (float t) {
            float u = 1 - t;
            return p1 + (u * u) * (p0 - p1) + (t * t) * (p2 - p1);
        }

        void forEvenSpacing (int n, std::function<void (int, float)> func) {
            auto stepSize = arcLength / static_cast<float> (n);

            int highBound = 1;
            for (int i = 0; i <= n; i++) {
                auto thisDist = i * stepSize;

                while (lenArr [highBound] < thisDist && highBound + 1 <= TABLE_SIZE)
                    highBound++;

                auto lowLen = lenArr [highBound - 1];
                auto highLen = lenArr [highBound];

                auto segT = (thisDist - lowLen) / (highLen - lowLen);

                func (i, Math::lerp (tArr [highBound - 1], tArr [highBound], segT));
            }
        }
    };
}