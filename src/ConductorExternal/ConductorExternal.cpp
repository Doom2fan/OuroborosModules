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

#include "ConductorExternal.hpp"

#include "../JsonUtils.hpp"

#include <fmt/format.h>

#include <string>

namespace OuroborosModules {
    rack::plugin::Model* modelConductorExternal = createModel<Modules::Conductor::ConductorExternalWidget> ("ConductorExternal");
}

namespace OuroborosModules::Modules::Conductor {
    static constexpr int NoteMapSteps = 24;

    /*
     * NoteMapState
     */
    json_t* ConductorExternalModule::NoteMapState::dataToJson () const {
        auto rootJ = json_object ();

        auto mappingsJ = json_object ();
        for (auto kvp : mappings)
            json_object_set_new_int (mappingsJ, std::to_string (kvp.first).c_str (), kvp.second);

        json_object_set_new (rootJ, "mappings", mappingsJ);

        return rootJ;
    }

    bool ConductorExternalModule::NoteMapState::dataFromJson (json_t* rootJ) {
        if (!json_is_object (rootJ))
            return false;

        auto mappingsJ = json_object_get (rootJ, "mappings");
        if (!json_is_object (mappingsJ))
            return false;

        NoteMappingsMap newMappings { };
        const char* noteKeyStr; json_t* mapIndexJ;
        json_object_foreach (mappingsJ, noteKeyStr, mapIndexJ) {
            if (!json_is_integer (mapIndexJ))
                return false;

            auto noteKey = std::stoi (noteKeyStr);
            auto mapIndex = json_integer_value (mapIndexJ);

            newMappings [noteKey] = mapIndex;
        }

        mappings = newMappings;
        mappingState = MappingState::None;

        return true;
    }

    void ConductorExternalModule::NoteMapState::reset () {
        mappingState = MappingState::None;
        currentNoteKey = 0;
    }

    void ConductorExternalModule::NoteMapState::clearMappings () {
        mappings.clear ();
    }

    std::optional<int> ConductorExternalModule::NoteMapState::getMapping (NoteMapKey noteKey) {
        if (auto it = mappings.find (noteKey); it != mappings.end ())
            return it->second;

        return std::nullopt;
    }

    void ConductorExternalModule::NoteMapState::setMapping (NoteMapKey noteKey, int index) {
        mappings [noteKey] = index;
    }

    void ConductorExternalModule::NoteMapState::removeMapping (NoteMapKey noteKey) {
        if (auto it = mappings.find (noteKey); it != mappings.end ())
            mappings.erase (it);
    }

    /*
     * ConductorExternalModule
     */
    ConductorExternalModule::ConductorExternalModule () {
        config (NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);

        // Params
        configSwitch (PARAM_MODE, 0.f, 2.f, 0.f, "Mode selector", { "Index", "Scroll", "Note Map" });

        configButton (PARAM_MAP_BUTTON, "Map");

        // Inputs
        configInput (INPUT_CV1, "CV 1");
        configInput (INPUT_CV2, "CV 2");
        configInput (INPUT_GATE, "Gate");

        // Lights
        configLight (LIGHT_CV1_ENABLED, "CV 1 Enabled");
        configLight (LIGHT_CV2_ENABLED, "CV 2 Enabled");

        clockLights = DSP::ClockDivider (128, rack::random::u32 ());

        switchMode (ModuleMode::Index);
    }

    json_t* ConductorExternalModule::dataToJson () {
        auto rootJ = ConductorExpander::dataToJson ();

        json_object_set_new_int (rootJ, "selectedPattern", selectedPattern);
        json_object_set_new_struct (rootJ, "noteMapState", noteMapState);

        return rootJ;
    }

    void ConductorExternalModule::dataFromJson (json_t* rootJ) {
        ConductorExpander::dataFromJson (rootJ);

        json_object_try_get_int (rootJ, "selectedPattern", selectedPattern);
        json_object_try_get_struct (rootJ, "noteMapState", noteMapState);

        checkMode ();
    }

    void ConductorExternalModule::onDataUpdated (const ConductorDataUpdatedEvent& e) {
        patternCount = e.patternCount;
        currentPattern = e.currentPattern;
        queuedPattern = e.queuedPattern;
    }

    void ConductorExternalModule::onEnabled () {
        activationCleanup ();
    }

    void ConductorExternalModule::onDisabled () {

    }

    void ConductorExternalModule::onReset (const ResetEvent& e) {
        ConductorExpander::onReset (e);

        noteMapState.clearMappings ();
        noteMapState.reset ();

        selectedPattern = 0;

        activationCleanup ();

        mode = ModuleMode::Invalid;
        checkMode ();
    }

    void ConductorExternalModule::activationCleanup () {
        mapButtonTrigger.reset ();

        cv1Trigger.reset ();
        cv2Trigger.reset ();
        gateTrigger.reset ();

        noteMapState.reset ();
        for (int idx = LIGHT_MAP_BUTTON; idx < LIGHT_MAP_BUTTON_LAST + 1; idx++)
            mapButtonLightState [idx - LIGHT_MAP_BUTTON] = false;
    }

    void ConductorExternalModule::checkMode () {
        auto modeInt = static_cast<int> (params [PARAM_MODE].getValue ());
        auto newMode = mode;
        switch (modeInt) {
            default:
            case 0: newMode = ModuleMode::Index; break;
            case 1: newMode = ModuleMode::Scroll; break;
            case 2: newMode = ModuleMode::NoteMap; break;
        }

        if (newMode != mode)
            switchMode (newMode);
    }

    void ConductorExternalModule::switchMode (ModuleMode newMode) {
        if (mode == newMode)
            return;

        mode = newMode;

        switch (mode) {
            case ModuleMode::Index:
                cv1Enabled = true;
                cv2Enabled = false;
                break;

            case ModuleMode::Scroll:
                cv1Enabled = true;
                cv2Enabled = true;
                break;

            case ModuleMode::NoteMap:
                cv1Enabled = true;
                cv2Enabled = false;
                break;

            // Unrecognized mode.
            default:
                cv1Enabled = cv2Enabled = false;
                break;
        }

        activationCleanup ();
    }

    void ConductorExternalModule::confirmQueue () {
        if (selectedPattern != queuedPattern)
            getCoreModule ()->requestEnqueue (selectedPattern);
        else
            getCoreModule ()->requestDequeue ();
    }

    void ConductorExternalModule::processActive (const ProcessArgs& args) {
        checkMode ();

        switch (mode) {
            case ModuleMode::Index: processIndex (args); break;
            case ModuleMode::Scroll: processScroll (args); break;
            case ModuleMode::NoteMap: processNoteMap (args); break;

            // Do nothing if our current mode is invalid.
            default: break;
        }

        // Ensure the selected pattern is in range.
        selectedPattern = std::clamp (selectedPattern, 0, patternCount - 1);
    }

    void ConductorExternalModule::processIndex (const ProcessArgs& args) {
        using Constants::TriggerThreshLow;
        using Constants::TriggerThreshHigh;

        auto indexFloat = std::clamp (inputs [INPUT_CV1].getVoltage () / 10.f, 0.f, 1.f);
        selectedPattern = patternFloatToInt (Math::rescale1 (indexFloat, 0, patternCount - 1));

        // Confirm
        if (gateTrigger.process (inputs [INPUT_GATE].getVoltage (), TriggerThreshLow, TriggerThreshHigh))
            confirmQueue ();
    }

    void ConductorExternalModule::processScroll (const ProcessArgs& args) {
        using Constants::TriggerThreshLow;
        using Constants::TriggerThreshHigh;

        // Scroll up
        if (cv1Trigger.process (inputs [INPUT_CV1].getVoltage (), TriggerThreshLow, TriggerThreshHigh))
            selectedPattern = std::min (selectedPattern + 1, patternCount - 1);

        // Scroll down
        if (cv2Trigger.process (inputs [INPUT_CV2].getVoltage (), TriggerThreshLow, TriggerThreshHigh))
            selectedPattern = std::max (selectedPattern - 1, 0);

        // Confirm
        if (gateTrigger.process (inputs [INPUT_GATE].getVoltage (), TriggerThreshLow, TriggerThreshHigh))
            confirmQueue ();
    }

    void ConductorExternalModule::processNoteMap (const ProcessArgs& args) {
        using Constants::TriggerThreshLow;
        using Constants::TriggerThreshHigh;

        // Calculate the selected index.
        auto prevNoteKey = noteMapState.currentNoteKey;
        noteMapState.currentNoteKey = static_cast<int> (std::round (inputs [INPUT_CV1].getVoltage () * NoteMapSteps));
        auto noteKeyChanged = noteMapState.currentNoteKey != prevNoteKey;

        // Handle mapping.
        if (mapButtonTrigger.process (params [PARAM_MAP_BUTTON].getValue ())) {
            switch (noteMapState.mappingState) {
                default:
                case MappingState::None: noteMapState.mappingState = MappingState::Mapping; break;
                case MappingState::Mapping: noteMapState.mappingState = MappingState::Unmapping; break;
                case MappingState::Unmapping: noteMapState.mappingState = MappingState::None; break;
            }
        }

        switch (noteMapState.mappingState) {
            default:
                noteMapState.mappingState = MappingState::None;
                [[fallthrough]];
            case MappingState::None: {
                if (noteKeyChanged) {
                    if (auto newPattern = noteMapState.getMapping (noteMapState.currentNoteKey))
                        selectedPattern = *newPattern;
                }

                mapButtonLightState [0] = false;
                mapButtonLightState [1] = false;
                break;
            }

            case MappingState::Mapping:
                selectedPattern = currentPattern;

                mapButtonLightState [0] = false;
                mapButtonLightState [1] = true;
                break;

            case MappingState::Unmapping:
                mapButtonLightState [0] = true;
                mapButtonLightState [1] = false;
                break;
        }

        // Confirm
        if (gateTrigger.process (inputs [INPUT_GATE].getVoltage (), TriggerThreshLow, TriggerThreshHigh)) {
            switch (noteMapState.mappingState) {
                default:
                case MappingState::None: {
                    if (auto newPattern = noteMapState.getMapping (noteMapState.currentNoteKey)) {
                        selectedPattern = *newPattern;
                        confirmQueue ();
                    }
                    break;
                }

                case MappingState::Mapping:
                    noteMapState.setMapping (noteMapState.currentNoteKey, currentPattern);
                    noteMapState.mappingState = MappingState::None;
                    break;

                case MappingState::Unmapping:
                    noteMapState.removeMapping (noteMapState.currentNoteKey);
                    noteMapState.mappingState = MappingState::None;
                    break;
            }
        }
    }

    void ConductorExternalModule::process (const ProcessArgs& args) {
        ConductorExpander::processExpander ();

        using Constants::TriggerThreshLow;
        using Constants::TriggerThreshHigh;

        auto prevEnabled = enabled;
        enabled = getCoreModule () != nullptr;
        if (!prevEnabled && enabled)
            onEnabled ();
        else if (prevEnabled && !enabled)
            onDisabled ();

        if (enabled)
            processActive (args);

        // Process lights.
        if (clockLights.process ()) {
            auto lightTime = args.sampleTime * clockLights.division;

            lights [LIGHT_CV1_ENABLED].setBrightness (boolToLight (cv1Enabled & enabled));
            lights [LIGHT_CV2_ENABLED].setBrightness (boolToLight (cv2Enabled & enabled));

            for (int idx = LIGHT_MAP_BUTTON; idx < LIGHT_MAP_BUTTON_LAST + 1; idx++)
                lights [idx].setBrightnessSmooth (boolToLight (mapButtonLightState [idx - LIGHT_MAP_BUTTON]), lightTime);
        }
    }
}