/*
 *  OuroborosModules
 *  Copyright (C) 2025 Chronos "phantombeta" Ouroboros
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

namespace OuroborosModules::DSP {
    struct ClockDivider {
        using TimerType = uint32_t;

        TimerType clock = 0;
        TimerType division = 1;

        ClockDivider () { }
        ClockDivider (TimerType division) : division (division) {}
        ClockDivider (TimerType newDivision, TimerType newClock) : clock (newClock % newDivision), division (newDivision) {}

        void reset () { clock = 0; }

        TimerType getDivision () { return division; }
        void setDivision (TimerType newDivision) {
            assert (newDivision > 0);
            division = newDivision;
        }

        TimerType getClock () { return clock; }
        void setClock (TimerType newClock) { clock = newClock % division; }

        bool process () {
            if (++clock >= division) {
                clock = 0;
                return true;
            }

            return false;
        }
    };
}