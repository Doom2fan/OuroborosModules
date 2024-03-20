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
    const std::string DefaultPlugSounds [] = {
        "res/sounds/Jack_Connect.wav",
        "res/sounds/Jack_Disconnect.wav",
    };

    std::string plugSound_GetConfigPath (MetaModule::PlugSound_Buffers buffer) {
        std::string retPath;
        switch (buffer) {
            case MetaModule::PLUGSOUND_CONNECT: retPath = pluginSettings.plugSound_ConnectSound; break;
            case MetaModule::PLUGSOUND_DISCONNECT: retPath = pluginSettings.plugSound_DisconnectSound; break;

            default: return "";
        }

        if (retPath == "<Default>")
            retPath = rack::asset::plugin (pluginInstance, DefaultPlugSounds [buffer]);

        return retPath;
    }

    void MetaModule::plugSound_SetSound (MetaModule::PlugSound_Buffers bufferIdx, std::string path, bool forceReload) {
        if (!forceReload && plugSound_Paths [bufferIdx] == path)
            return;

        plugSound_Paths [bufferIdx] = path;
        plugSound_LoadStatus [bufferIdx] = plugSound_Buffers [bufferIdx]->load (path);

        // TODO: Report sample loading errors.
        if (plugSound_LoadStatus [bufferIdx] != Audio::AudioSample::LoadStatus::Success)
            plugSound_Buffers [bufferIdx]->clear ();
    }

    void MetaModule::plugSound_Process (const ProcessArgs& args) {
        auto enabled = pluginSettings.plugSound_Enable;
        auto prevEnabled = plugSound_PrevEnabled;
        plugSound_PrevEnabled = enabled;

        auto requestGlobalLoad = enabled && !prevEnabled;
        if (!enabled && prevEnabled) {
            for (int i = 0; i < PLUGSOUND_LENGTH; i++)
                plugSound_Buffers [i]->clear ();
        }

        if (!enabled)
            return;

        if (args.frame % plugSound_SampleCheckInterval != 0 &&
            plugSound_RequestLoad == PLUGSOUND_LENGTH &&
            !requestGlobalLoad)
            return;

        for (int i = 0; i < PLUGSOUND_LENGTH; i++) {
            auto bufferIdx = (PlugSound_Buffers) i;
            auto requestLoad = requestGlobalLoad || bufferIdx == plugSound_RequestLoad;
            plugSound_SetSound (bufferIdx, plugSound_GetConfigPath (bufferIdx), requestLoad);
        }

        plugSound_RequestLoad = PLUGSOUND_LENGTH;
    }

    void MetaModule::plugSound_ProcessAudio (const ProcessArgs& args, float& audioLeft, float& audioRight) {
        float mixAudioLeft = 0.f, mixAudioRight = 0.f;
        float chanAudioLeft, chanAudioRight;
        for (int i = 0; i < PLUGSOUND_LENGTH; i++) {
            if (!plugSound_Channels [i].process (chanAudioLeft, chanAudioRight))
                continue;

            mixAudioLeft += chanAudioLeft;
            mixAudioRight += chanAudioRight;
        }

        auto gain = 5.f * std::pow (pluginSettings.plugSound_Volume, 2.f);
        audioLeft += mixAudioLeft * gain;
        audioRight += mixAudioRight * gain;
    }
}
}