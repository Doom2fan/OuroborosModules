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

#include "MetaModule.hpp"

namespace OuroborosModules {
namespace MetaModule {
    void MetaModule::audio_Reset () {
        for (int i = 0; i < PLUGSOUND_LENGTH; i++)
            plugSound_Channels [i].reset ();
    }

    void MetaModule::audio_Process (const ProcessArgs& args) {
        // Check if anything's connected.
        bool outputLeft = outputs [OUTPUTL_OUTPUT].isConnected ();
        bool outputRight = outputs [OUTPUTR_OUTPUT].isConnected ();

        if (!outputLeft && !outputRight) {
            if (outputtingAudio) {
                audio_Reset ();
                outputtingAudio = false;
            }

            return;
        } else
            outputtingAudio = true;

        float audioLeft = inputs [INPUTL_INPUT].isConnected () ? inputs [INPUTL_INPUT].getVoltage () : 0.f;
        float audioRight = inputs [INPUTR_INPUT].isConnected () ? inputs [INPUTR_INPUT].getVoltage () : audioLeft;

        (this->*premuter_Func) (args.sampleTime, audioLeft, audioRight);
        plugSound_ProcessAudio (args, audioLeft, audioRight);

        if (outputLeft ) outputs [OUTPUTL_OUTPUT].setVoltage (audioLeft);
        if (outputRight) outputs [OUTPUTR_OUTPUT].setVoltage (audioRight);
    }
}
}