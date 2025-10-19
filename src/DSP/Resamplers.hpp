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
#include "Filters.hpp"

#include <array>

namespace OuroborosModules::DSP {
    template<typename T>
    struct Decimator {
        Decimator () { }
        virtual ~Decimator () { }

        virtual void setParams (int factor) = 0;
        virtual T process (const T* inputBuffer) = 0;
    };

    template<typename T>
    struct Butterworth6PDecimator : Decimator<T> {
      private:
        int oversampleFactor = 1;
        Butterworth6P<T> filter = Butterworth6P<T> ();

      public:
        void setParams (int newOversampleFactor) override {
            oversampleFactor = newOversampleFactor;
            filter.setCutoffFreq (1.f / (newOversampleFactor * 4));
        }

        T process (const T* inputBuffer) override {
            for (int i = 0; i < oversampleFactor - 1; ++i)
                filter.process (inputBuffer [i]);
            return filter.process (inputBuffer [oversampleFactor - 1]);
        }
    };

    template<typename T>
    struct Interpolator {
        Interpolator () { }
        virtual ~Interpolator () { }

        virtual void setParams (int factor) = 0;
        virtual void process (T* outputBuffer, T input) = 0;
    };

    template<typename T>
    struct Butterworth6PInterpolator : Interpolator<T> {
      private:
        int oversampleFactor = 1;
        Butterworth6P<T> filter = Butterworth6P<T> ();

      public:
        void setParams (int factor) override {
            oversampleFactor = factor;
            filter.setCutoffFreq (1.f / (factor * 4));
        }

        void process (T* outputBuffer, T input) override {
            outputBuffer [0] = filter.process (input * oversampleFactor);

            auto zero = T (0);
            for (int i = 1; i < oversampleFactor; ++i)
                outputBuffer [i] = filter.process (zero);
        }
    };
}