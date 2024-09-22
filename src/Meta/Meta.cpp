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

#include "../JsonUtils.hpp"

namespace OuroborosModules {
    rack::plugin::Model* modelMeta = createModel<Modules::Meta::MetaWidget> ("MetaModule");
}

namespace OuroborosModules {
namespace Modules {
namespace Meta {
    MetaModule::MetaModule () {
        config (PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);

        // Configure audio inputs and outputs.
        configInput (INPUT_LEFT, "Left");
        configInput (INPUT_RIGHT, "Right");
        configOutput (OUTPUT_LEFT, "Left");
        configOutput (OUTPUT_RIGHT, "Right");
        configBypass (INPUT_LEFT, OUTPUT_LEFT);
        configBypass (INPUT_RIGHT, OUTPUT_RIGHT);

        premuter_Func = &MetaModule::premuter_Process;
    }

    json_t* MetaModule::dataToJson () {
        auto rootJ = ModuleBase::dataToJson ();

        // Premuter
        json_object_set_new_float (rootJ, "premuter_SelectedTime", premuter_SelectedTime);

        return rootJ;
    }

    void MetaModule::dataFromJson (json_t* rootJ) {
        ModuleBase::dataFromJson (rootJ);

        // Premuter
        json_object_try_get_float (rootJ, "premuter_SelectedTime", premuter_SelectedTime);
    }

    void MetaModule::premuter_Process (float sampleTime, float& audioLeft, float& audioRight) {
        const float popBlockerTime = .25f;
        float totalTime = premuter_SelectedTime + popBlockerTime;

        float volume = rack::math::clamp ((premuter_SampleTime - premuter_SelectedTime) / popBlockerTime, 0.f, 1.f);
        volume *= volume;

        audioLeft *= volume;
        audioRight *= volume;

        if (premuter_SampleTime > totalTime)
            premuter_Func = &MetaModule::premuter_Passthrough;

        premuter_SampleTime += sampleTime;
    }

    void MetaModule::process (const ProcessArgs& args) {
        if (isBypassed ())
            return;

        bool cableConnected, cableDisconnected;
        cables_Process (args, cableConnected, cableDisconnected);

        if (cableConnected)
            plugSound_Channels [PLUGSOUND_CONNECT].play ();
        if (cableDisconnected)
            plugSound_Channels [PLUGSOUND_DISCONNECT].play ();

        audio_Process (args);
    }

    void MetaModule::onSampleRateChange (const SampleRateChangeEvent& e) {
        Module::onSampleRateChange (e);

        for (int i = 0; i < PLUGSOUND_LENGTH; i++)
            plugSound_Channels [i].onSampleRateChange (e.sampleRate);
    }

    void MetaModule::onUnBypass (const UnBypassEvent& e) {
        Module::onUnBypass (e);

        premuter_Func = &MetaModule::premuter_Passthrough;
        for (int i = 0; i < PLUGSOUND_LENGTH; i++)
            plugSound_Channels [i].reset ();
    }
}
}
}