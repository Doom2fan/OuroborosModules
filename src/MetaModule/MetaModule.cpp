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
#include "../JsonUtils.hpp"

Model* modelMetaModule = createModel<MetaModule, MetaModuleWidget> ("MetaModule");
constexpr int plugSound_SampleCheckRate = 60;

MetaModule::MetaModule () {
    config (PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);

    // Configure audio inputs and outputs.
    configInput (INPUTL_INPUT, "Left input");
    configInput (INPUTR_INPUT, "Right input");
    configOutput (OUTPUTL_OUTPUT, "Left output");
    configOutput (OUTPUTR_OUTPUT, "Right output");
    configBypass (INPUTL_INPUT, OUTPUTL_OUTPUT);
    configBypass (INPUTR_INPUT, OUTPUTR_OUTPUT);

    premuter_Func = &MetaModule::premuter_Process;

    for (int i = 0; i < PLUGSOUND_LENGTH; i++)
        plugSound_Buffers [i] = std::make_shared<AudioSample> ();

    for (int i = 0; i < PLUGSOUND_LENGTH; i++)
        plugSound_Channels [i].load (plugSound_Buffers [i]);

    CalcIntervals ();
}

MetaModule::~MetaModule () {
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

    float volume = math::clamp ((premuter_SampleTime - premuter_SelectedTime) / popBlockerTime, 0.f, 1.f);
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

    plugSound_Process (args);

    if (cableConnected)
        plugSound_Channels [PLUGSOUND_CONNECT].play ();
    if (cableDisconnected)
        plugSound_Channels [PLUGSOUND_DISCONNECT].play ();

    audio_Process (args);
}

void MetaModule::CalcIntervals () {
    auto engineSampleRate = APP->engine->getSampleRate ();
    plugSound_SampleCheckInterval = engineSampleRate / plugSound_SampleCheckRate;
    cable_CheckInterval = engineSampleRate / pluginSettings.cables_CalcRate;
}

void MetaModule::onSampleRateChange (const SampleRateChangeEvent& e) {
    Module::onSampleRateChange (e);

    for (int i = 0; i < PLUGSOUND_LENGTH; i++) {
        plugSound_Buffers [i]->onSampleRateChange (e.sampleRate);
        plugSound_Channels [i].onSampleRateChange (e.sampleRate);
    }

    CalcIntervals ();
}

void MetaModule::onUnBypass (const UnBypassEvent& e) {
    Module::onUnBypass (e);

    premuter_Func = &MetaModule::premuter_Passthrough;
    for (int i = 0; i < PLUGSOUND_LENGTH; i++)
        plugSound_Channels [i].reset ();
}