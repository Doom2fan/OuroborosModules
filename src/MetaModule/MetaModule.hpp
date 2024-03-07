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

#include "../PluginDef.hpp"
#include "../ModuleBase.hpp"
#include "../SampleChannel.hpp"
#include "../Utils.hpp"
#include "../UI/WidgetBase.hpp"
#include "../UI/ImageWidget.hpp"

struct MetaModule : ModuleBase {
    enum ParamId {
        PARAMS_LEN
    };
    enum InputId {
        INPUTL_INPUT,
        INPUTR_INPUT,
        INPUTS_LEN
    };
    enum OutputId {
        OUTPUTL_OUTPUT,
        OUTPUTR_OUTPUT,
        OUTPUTS_LEN
    };
    enum LightId {
        LIGHTS_LEN
    };

    enum PlugSound_Buffers {
        PLUGSOUND_CONNECT,
        PLUGSOUND_DISCONNECT,
        PLUGSOUND_LENGTH,
    };

    // Common data
    bool outputtingAudio = false;

    // Cable data
    int cable_CheckInterval = 0;
    int cable_PrevCableCount = 0;
    bool cable_HadIncompleteCable = false;

    // Plug sound data
    bool plugSound_PrevEnabled = false;
    int plugSound_SampleCheckInterval = 0;
    int plugSound_RequestLoad = -1;
    AudioSample::LoadStatus plugSound_LoadStatus [PLUGSOUND_LENGTH];
    std::string plugSound_Paths [PLUGSOUND_LENGTH];
    std::shared_ptr<AudioSample> plugSound_Buffers [PLUGSOUND_LENGTH];
    SampleChannel plugSound_Channels [PLUGSOUND_LENGTH];

    // Premuter data
    float premuter_SampleTime = 0.f;
    float premuter_SelectedTime = 1.5f;
    void (MetaModule::*premuter_Func) (float sampleTime, float& audioLeft, float& audioRight);

    MetaModule ();
    ~MetaModule ();

    json_t* dataToJson () override;
    void dataFromJson (json_t* rootJ) override;

    void CalcIntervals ();

    void cables_Process (const ProcessArgs& args, bool& cableConnected, bool& cableDisconnected);

    void premuter_Process (float sampleTime, float& audioLeft, float& audioRight);
    void premuter_Passthrough (float sampleTime, float& audioLeft, float& audioRight) { }

    void plugSound_Enable (bool enable);
    void plugSound_SetSound (PlugSound_Buffers bufferIdx, std::string path, bool forceReload);
    void plugSound_Process (const ProcessArgs& args);
    void plugSound_ProcessAudio (const ProcessArgs& args, float& audioLeft, float& audioRight);

    void audio_Reset ();
    void audio_Process (const ProcessArgs& args);

    void process (const ProcessArgs& args) override;
    void onSampleRateChange (const SampleRateChangeEvent& e) override;
    void onUnBypass (const UnBypassEvent& e) override;
};

struct MetaModuleWidget : ModuleWidgetBase<MetaModuleWidget, MetaModule> {
    ImageWidget* emblemWidget;

    MetaModuleWidget (MetaModule* module);

    void setEmblem (EmblemKind emblem);
    void setEmblem ();

  protected:
    void initializeWidget () override;

    void updateEmblem (ThemeKind theme, EmblemKind emblem);
    void onChangeTheme (ThemeKind kind) override;
    void onChangeEmblem (EmblemKind emblem) override;
    void createPluginSettingsMenu (MetaModuleWidget* widget, rack::ui::Menu* menu) override;
    void appendContextMenu (rack::ui::Menu* menu) override;
};