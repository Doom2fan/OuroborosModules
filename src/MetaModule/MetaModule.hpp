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

#pragma once

#include "../CableHandler.hpp"
#include "../ModuleBase.hpp"
#include "../PluginDef.hpp"
#include "../SampleChannel.hpp"
#include "../UI/ImageWidget.hpp"
#include "../UI/WidgetBase.hpp"
#include "../Utils.hpp"

namespace OuroborosModules {
namespace MetaModule {
    struct SampleSlot {
      private:
        std::string samplePath = "";
        std::string sampleLoadPath = "";
        std::shared_ptr<Audio::AudioSample> sample = nullptr;
        std::shared_ptr<Audio::AudioSample> sampleLoad = nullptr;

        Audio::SampleChannel sampleChannel = Audio::SampleChannel ();

        std::atomic<bool> loadRequested = false;

        void checkLoad ();

      public:
        bool load (std::string newSamplePath, bool forceReload, bool reportErrors);

        void reset () { checkLoad (); sampleChannel.reset (); }
        void play () { checkLoad (); sampleChannel.play (); }
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

        enum PlugSound_Channels {
            PLUGSOUND_CONNECT,
            PLUGSOUND_DISCONNECT,
            PLUGSOUND_LENGTH,
        };

        // Common data
        bool outputtingAudio = false;

        // Cable data
        std::atomic<bool> cables_NewConnected = false;
        std::atomic<bool> cables_NewDisconnected = false;

        // Plug sound data
        std::atomic<bool> plugSound_PrevEnabled = false;
        SampleSlot plugSound_Channels [PLUGSOUND_LENGTH];

        // Premuter data
        float premuter_SampleTime = 0.f;
        float premuter_SelectedTime = 1.5f;
        void (MetaModule::*premuter_Func) (float sampleTime, float& audioLeft, float& audioRight);

        MetaModule ();

        json_t* dataToJson () override;
        void dataFromJson (json_t* rootJ) override;

        void cables_Process (const ProcessArgs& args, bool& cableConnected, bool& cableDisconnected);

        void premuter_Process (float sampleTime, float& audioLeft, float& audioRight);
        void premuter_Passthrough (float sampleTime, float& audioLeft, float& audioRight) { }

        void plugSound_ProcessAudio (const ProcessArgs& args, float& audioLeft, float& audioRight);

        void audio_Reset ();
        void audio_Process (const ProcessArgs& args);

        void process (const ProcessArgs& args) override;
        void onSampleRateChange (const SampleRateChangeEvent& e) override;
        void onUnBypass (const UnBypassEvent& e) override;
    };

    struct MetaModuleWidget : Widgets::ModuleWidgetBase<MetaModuleWidget, MetaModule> {
      private:
        Widgets::ImageWidget* emblemWidget;
        std::shared_ptr<CableHandler> cables_Handler;

      public:
        MetaModuleWidget (MetaModule* module);

      protected:
        void initializeWidget () override;

        void step () override;
        void plugSound_CheckChannels ();

        void updateCableHandler ();
        void updateEmblem (ThemeId themeId, EmblemId emblemId);
        void onChangeTheme (ThemeId themeId) override;
        void onChangeEmblem (EmblemId emblemId) override;
        void createPluginSettingsMenu (rack::ui::Menu* menu) override;
        void appendContextMenu (rack::ui::Menu* menu) override;
    };
}
}