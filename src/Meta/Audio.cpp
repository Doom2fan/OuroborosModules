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

#include "Meta.hpp"

namespace OuroborosModules::Modules::Meta {
    void MetaModule::audio_Reset () {
        for (int i = 0; i < METASOUNDS_LENGTH; i++)
            metaSounds_Channels [i].reset ();
    }

    void MetaModule::audio_Process (const ProcessArgs& args) {
        // Check if anything's connected.
        bool outputLeft = isOutputConnected (OUTPUT_LEFT);
        bool outputRight = isOutputConnected (OUTPUT_RIGHT);

        if (!outputLeft && !outputRight) {
            if (outputtingAudio) {
                audio_Reset ();
                outputtingAudio = false;
            }

            return;
        } else
            outputtingAudio = true;

        float audioLeft = getInputNormal (INPUT_LEFT, 0.f);
        float audioRight = getInputNormal (INPUT_RIGHT, audioLeft);

        (this->*premuter_Func) (args.sampleTime, audioLeft, audioRight);
        metaSounds_Process (args);
        metaSounds_ProcessAudio (args, audioLeft, audioRight);

        if (outputLeft ) setOutput (OUTPUT_LEFT, audioLeft);
        if (outputRight) setOutput (OUTPUT_RIGHT, audioRight);
    }
}