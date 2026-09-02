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
#include "ModuleBase.hpp"
#include "Math.hpp"

namespace OuroborosModules {
    template<bool TClamp = true>
    struct AutoAttenuverter {
      private:
        float paramValue;
        float minValue;
        float maxValue;

        float attenuverter;

      public:
        AutoAttenuverter () {
            init (0, 0, 1, 1, 0, 1); // Just to ensure it doesn't blow up
        }

        AutoAttenuverter (ModuleBase* module, int paramId, int attenId, float inRange, float endRange = std::numeric_limits<float>::infinity ()) {
            assert (module != nullptr);

            auto paramQuantity = module->getParamQuantity (paramId);
            init (
                module->getParam (paramId),
                module->getParam (attenId), inRange, endRange,
                paramQuantity->minValue, paramQuantity->maxValue
            );
        }

        AutoAttenuverter (ModuleBase* module, int paramId, int attenId, float inRange, float minVal, float maxVal, float endRange = std::numeric_limits<float>::infinity ()) {
            assert (module != nullptr);
            init (
                module->getParam (paramId),
                module->getParam (attenId), inRange, endRange,
                minVal, maxVal
            );
        }

        AutoAttenuverter (float paramVal, float attenVal, float inRange, float minVal, float maxVal, float endRange = std::numeric_limits<float>::infinity ()) {
            init (
                paramVal,
                attenVal, inRange, endRange,
                minVal, maxVal
            );
        }

        void init (
            float paramVal,
            float attenVal, float inRange, float endRange,
            float minVal, float maxVal
        ) {
            paramValue = paramVal;
            minValue = minVal;
            maxValue = maxVal;

            if (std::isinf (endRange))
                endRange = std::abs (maxValue - minValue);

            attenuverter = attenVal / inRange * endRange;
        }

        float getParamValue () { return paramValue; }
        void setParamValue (float value) { paramValue = value; }

        template<typename T>
        T process (T value) {
            auto ret = T (paramValue) + Math::fpClean (value) * attenuverter;
            if (TClamp)
                ret = rack::simd::clamp (ret, T (minValue), T (maxValue));

            return ret;
        }
    };
}