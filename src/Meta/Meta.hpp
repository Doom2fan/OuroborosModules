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

#include "../MetaHandler.hpp"
#include "../DSP/ClockDivider.hpp"
#include "../ModuleBase.hpp"
#include "../PluginDef.hpp"
#include "../SampleChannel.hpp"
#include "../UI/CommonWidgets.hpp"
#include "../UI/WidgetBase.hpp"
#include "../Utils.hpp"

namespace OuroborosModules::Modules::Meta {
    struct SampleSlot {
      private:
        int channelIdx = 0;

        float audioGain = 1.f;

        std::shared_ptr<Audio::AudioSample> sample = nullptr;
        Audio::SampleChannel sampleChannel = Audio::SampleChannel ();

      public:
        void init (int channelIdx) { this->channelIdx = channelIdx; }

        void update ();

        void reset () { sampleChannel.reset (); }
        void play () { sampleChannel.play (); }
        bool process (float& audioLeft, float& audioRight);

        void onSampleRateChange (int sampleRate) { sampleChannel.onSampleRateChange (sampleRate); }
    };

    struct MetaModule : ModuleBase {
        enum ParamId {
            PARAMS_LEN
        };
        enum InputId {
            INPUT_LEFT,
            INPUT_RIGHT,
            INPUTS_LEN
        };
        enum OutputId {
            OUTPUT_LEFT,
            OUTPUT_RIGHT,
            OUTPUTS_LEN
        };
        enum LightId {
            LIGHTS_LEN
        };

        enum MetaSounds_Channels {
            METASOUNDS_CABLECONNECT,
            METASOUNDS_CABLEDISCONNECT,
            METASOUNDS_LENGTH,
        };

        // Common data
        bool outputtingAudio = false;

        // Cable data
        std::atomic<bool> cables_NewConnected = false;
        std::atomic<bool> cables_NewDisconnected = false;

        // Meta sound data
        DSP::ClockDivider clockMetaSoundSettings;
        SampleSlot metaSounds_Channels [METASOUNDS_LENGTH];

        // Premuter data
        float premuter_SampleTime = 0.f;
        float premuter_SelectedTime = 1.5f;
        void (MetaModule::*premuter_Func) (float sampleTime, float& audioLeft, float& audioRight);

        MetaModule ();

        json_t* dataToJson () override;
        void dataFromJson (json_t* rootJ) override;

        void cables_Process (const ProcessArgs& args);

        void premuter_Process (float sampleTime, float& audioLeft, float& audioRight);
        void premuter_Passthrough (float sampleTime, float& audioLeft, float& audioRight) { }

        void metaSounds_Process (const ProcessArgs& args);
        void metaSounds_ProcessAudio (const ProcessArgs& args, float& audioLeft, float& audioRight);

        void audio_Reset ();
        void audio_Process (const ProcessArgs& args);

        void process (const ProcessArgs& args) override;
        void onSampleRateChange (const SampleRateChangeEvent& e) override;
        void onUnBypass (const UnBypassEvent& e) override;
    };

    struct MetaWidget : Widgets::ModuleWidgetBase<MetaWidget, MetaModule> {
      private:
        Widgets::EmblemWidget* emblemWidget = nullptr;
        std::shared_ptr<MetaHandler> metaHandler = nullptr;
        bool metaSounds_PrevEnabled = false;

      public:
        MetaWidget (MetaModule* module);

      protected:
        void initializeWidget () override;

        void step () override;

        void updateMetaHandler ();
        void onChangeEmblem (EmblemId emblemId) override;
        void createPluginSettingsMenu (rack::ui::Menu* menu) override;
        void appendContextMenu (rack::ui::Menu* menu) override;
    };

    struct MetaSoundData {
      private:
        MetaModule::MetaSounds_Channels channelIdx = (MetaModule::MetaSounds_Channels) 0;

        std::string name;
        std::string defaultPath;
        SoundSettings* settings;

        std::string samplePath = "";
        std::shared_ptr<Audio::AudioSample> audioSample = nullptr;

      public:
        MetaSoundData (std::string name, std::string defaultPath, SoundSettings* settings)
            : name (name), defaultPath (defaultPath), settings (settings) { }

        void init (MetaModule::MetaSounds_Channels channel);

        const std::string& getName () { return name; }

        void setEnabled (bool enabled);
        bool isEnabled () { return settings != nullptr ? settings->enabled : true; }

        float* getVolumePtr () { return settings != nullptr ? &settings->volume : nullptr; }
        float getVolume () { return settings != nullptr ? settings->volume : 1.f; }

        std::shared_ptr<Audio::AudioSample> getAudio () { return audioSample; }

        bool changeSample (std::string path, bool forceReload, bool reportErrors);
        void tryChangePath (std::string path, bool forceReload, bool reportErrors);

        static void global_OnSampleRateChange (int newSampleRate);
    };

    void metaSounds_Init ();
    void metaSounds_Refresh ();
    MetaSoundData* metaSounds_GetData (MetaModule::MetaSounds_Channels channel);
}