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

#include "Meta.hpp"

#include <osdialog.h>

namespace OuroborosModules::Modules::Meta {
    const std::string DefaultPlugSounds [] = {
        "res/sounds/Jack_Connect.wav",
        "res/sounds/Jack_Disconnect.wav",
    };

    std::string plugSound_GetConfigPath (MetaModule::PlugSound_Channels buffer) {
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

    void SampleSlot::checkLoad () {
        if (!loadRequested)
            return;

        sample = sampleLoad;
        samplePath = sampleLoadPath;

        sampleLoad = nullptr;
        sampleLoadPath = "";

        sampleChannel.load (sample);

        loadRequested = false;
    }

    bool SampleSlot::load (std::string newSamplePath, bool forceReload, bool reportErrors) {
        if (loadRequested)
            return false;

        if (!forceReload && samplePath == newSamplePath)
            return true;

        if (newSamplePath.empty ()) {
            sampleLoadPath = newSamplePath;
            sampleLoad = nullptr;

            loadRequested = true;
            return true;
        }

        auto newSample = std::make_shared<Audio::AudioSample> ();
        auto loadResult = newSample->load (newSamplePath);

        if (loadResult != Audio::AudioSample::LoadStatus::Success) {
            if (reportErrors) {
                auto errorMessage = getErrorMessage (loadResult, newSamplePath);
                osdialog_message (OSDIALOG_WARNING, OSDIALOG_OK, errorMessage.c_str ());
            }
            return false;
        }

        sampleLoadPath = newSamplePath;
        sampleLoad = newSample;

        loadRequested = true;
        return true;
    }

    bool SampleSlot::process (float& audioLeft, float& audioRight) {
        auto ret = sampleChannel.process (audioLeft, audioRight);
        if (!ret)
            checkLoad ();

        return ret;
    }

    void MetaWidget::plugSound_CheckChannels () {
        auto enabled = pluginSettings.plugSound_Enable;
        auto prevEnabled = module->plugSound_PrevEnabled.exchange (enabled);

        auto requestGlobalLoad = enabled && !prevEnabled;
        if (!enabled && prevEnabled) {
            for (int i = 0; i < MetaModule::PLUGSOUND_LENGTH; i++)
                module->plugSound_Channels [i].load ("", true, false);
        }

        if (!enabled)
            return;

        for (int i = 0; i < MetaModule::PLUGSOUND_LENGTH; i++) {
            auto bufferIdx = (MetaModule::PlugSound_Channels) i;
            module->plugSound_Channels [i].load (plugSound_GetConfigPath (bufferIdx), requestGlobalLoad, false);
        }
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