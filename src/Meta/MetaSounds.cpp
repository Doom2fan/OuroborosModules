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
    std::atomic<int> metaSoundsSampleRate = 48000;
    MetaSoundData metaSoundsData [MetaModule::METASOUNDS_LENGTH] = {
        MetaSoundData ("Cable plugged", "res/sounds/Jack_Connect.wav", &pluginSettings.metaSounds_CablePlugged),
        MetaSoundData ("Cable unplugged", "res/sounds/Jack_Disconnect.wav", &pluginSettings.metaSounds_CableUnplugged),
        MetaSoundData ("Module placed", "res/sounds/Module_Place.wav", &pluginSettings.metaSounds_ModulePlaced),
        MetaSoundData ("Module removed", "res/sounds/Module_Remove.wav", &pluginSettings.metaSounds_ModuleRemoved),
    };

    void metaSounds_Init () {
        for (int i = 0; i < MetaModule::METASOUNDS_LENGTH; i++)
            metaSoundsData [i].init ((MetaModule::MetaSounds_Channels) i);
    }

    void metaSounds_Refresh () {
        for (int i = 0; i < MetaModule::METASOUNDS_LENGTH; i++) {
            auto& data = metaSoundsData [i];
            data.setEnabled (data.isEnabled ());
        }
    }

    MetaSoundData* metaSounds_GetData (MetaModule::MetaSounds_Channels channel) {
        if (channel < 0 || channel >= MetaModule::METASOUNDS_LENGTH) {
            LOG_WARN (FMT_STRING ("Invalid meta sound channel {}"), (int) channel);
            return nullptr;
        }

        return &(metaSoundsData [channel]);
    }

    void MetaSoundData::init (MetaModule::MetaSounds_Channels channel) {
        if (channel < 0 || channel >= MetaModule::METASOUNDS_LENGTH) {
            LOG_WARN (FMT_STRING ("Invalid meta sound channel {}"), (int) channel);
            return;
        }

        this->channelIdx = channel;

        if (settings == nullptr) {
            LOG_WARN (FMT_STRING ("Meta sound channel {} missing settings pointer"), (int) channel);
            return;
        }
    }

    void MetaSoundData::setEnabled (bool enabled) {
        if (settings == nullptr)
            return;

        settings->enabled = enabled;
        if (!enabled)
            changeSample ("", false, false);
        else
            changeSample (settings->path, false, false);
    }

    bool MetaSoundData::changeSample (std::string path, bool forceReload, bool reportErrors) {
        if (path == samplePath && !forceReload)
            return false;

        auto realPath = path;
        if (path == Constants::MetaSound_DefaultMarker)
            realPath = rack::asset::plugin (pluginInstance, defaultPath);

        if (realPath.empty ()) {
            audioSample = nullptr;
            samplePath = "";

            return true;
        }

        std::shared_ptr<Audio::AudioSample> newSample;
        auto loadResult = Audio::AudioSample::loadShared (realPath, metaSoundsSampleRate.load (), newSample);

        if (loadResult != Audio::AudioSample::LoadStatus::Success) {
            if (reportErrors) {
                auto errorMessage = getErrorMessage (loadResult, realPath);
                osdialog_message (OSDIALOG_WARNING, OSDIALOG_OK, errorMessage.c_str ());
            }
            return false;
        }

        audioSample = newSample;
        samplePath = realPath;
        return true;
    }

    void MetaSoundData::tryChangePath (std::string path, bool forceReload, bool reportErrors) {
        if (path == "")
            return;

        if (changeSample (path, forceReload, reportErrors))
            settings->path = path;
    }

    void MetaSoundData::global_OnSampleRateChange (int newSampleRate) {
        if (newSampleRate == metaSoundsSampleRate.exchange (newSampleRate))
            return;

        for (int i = 0; i < MetaModule::METASOUNDS_LENGTH; i++) {
            auto& data = metaSoundsData [i];
            data.audioSample = data.audioSample->withSampleRate (newSampleRate, true);
        }
    }

    void MetaModule::metaSounds_Process (const ProcessArgs& args) {
        if (cables_NewConnected.exchange (false))
            metaSounds_Channels [METASOUNDS_CABLECONNECT].play ();
        if (cables_NewDisconnected.exchange (false))
            metaSounds_Channels [METASOUNDS_CABLEDISCONNECT].play ();

        if (modules_NewPlaced.exchange (false))
            metaSounds_Channels [METASOUNDS_MODULEPLACED].play ();
        if (modules_NewRemoved.exchange (false))
            metaSounds_Channels [METASOUNDS_MODULEREMOVED].play ();
    }

    void MetaModule::metaSounds_ProcessAudio (const ProcessArgs& args, float& audioLeft, float& audioRight) {
        if (clockMetaSoundSettings.process ()) {
            for (int i = 0; i < MetaModule::METASOUNDS_LENGTH; i++)
                metaSounds_Channels [i].update ();
        }

        float mixAudioLeft = 0.f, mixAudioRight = 0.f;
        float chanAudioLeft, chanAudioRight;
        for (int i = 0; i < METASOUNDS_LENGTH; i++) {
            if (!metaSounds_Channels [i].process (chanAudioLeft, chanAudioRight))
                continue;

            mixAudioLeft += chanAudioLeft;
            mixAudioRight += chanAudioRight;
        }

        audioLeft += mixAudioLeft;
        audioRight += mixAudioRight;
    }
}