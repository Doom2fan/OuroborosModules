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

#include "../Conductor/Common.hpp"

#include "../DSP/ClockDivider.hpp"
#include "../PluginDef.hpp"
#include "../UI/CommonWidgets.hpp"
#include "../UI/WidgetBase.hpp"

#include <unordered_map>

namespace OuroborosModules::Modules::Conductor {
    struct ConductorExternalModule : ConductorExpander {
        enum class ModuleMode {
            Invalid,

            Index,
            Scroll,
            NoteMap,
        };

        enum class MappingState {
            None,
            Mapping,
            Unmapping,
        };

        enum ParamIds {
            PARAM_MODE,
            PARAM_MAP_BUTTON,

            PARAMS_LEN
        };
        enum InputIds {
            INPUT_CV1,
            INPUT_CV2,
            INPUT_GATE,

            INPUTS_LEN
        };
        enum OutputIds {
            OUTPUTS_LEN
        };
        enum LightIds {
            LIGHT_CV1_ENABLED,
            LIGHT_CV2_ENABLED,

            ENUMS (LIGHT_MAP_BUTTON, 2),

            LIGHTS_LEN
        };

        //typedef int NoteMapKey;
        using NoteMapKey = int32_t;
        using NoteMappingsMap = std::unordered_map<NoteMapKey, int>;
        struct NoteMapState {
          private:
            NoteMappingsMap mappings;

          public:
            MappingState mappingState = MappingState::None;
            NoteMapKey currentNoteKey = 0;

            void reset ();
            void clearMappings ();

            std::optional<int> getMapping (NoteMapKey noteKey);
            void setMapping (NoteMapKey noteKey, int index);
            void removeMapping (NoteMapKey noteKey);

            json_t* dataToJson () const;
            bool dataFromJson (json_t* rootJ);
        };

        ModuleMode mode = ModuleMode::Invalid;

        // State
        bool enabled = false;

        int patternCount = 0;
        int currentPattern = 0;
        int queuedPattern = 0;

        int selectedPattern = 0;

        bool cv1Enabled = false;
        bool cv2Enabled = false;
        bool mapButtonLightState [LIGHT_MAP_BUTTON_LAST - LIGHT_MAP_BUTTON + 1];

        NoteMapState noteMapState;

        // Triggers
        rack::dsp::SchmittTrigger mapButtonTrigger;

        rack::dsp::SchmittTrigger cv1Trigger;
        rack::dsp::SchmittTrigger cv2Trigger;
        rack::dsp::SchmittTrigger gateTrigger;

        // Clock dividers
        DSP::ClockDivider clockLights;

        ConductorExternalModule ();

        json_t* dataToJson () override;
        void dataFromJson (json_t* rootJ) override;

        void process (const ProcessArgs& args) override;

        void onDataUpdated (const ConductorDataUpdatedEvent& coreData) override;

        void onReset (const ResetEvent& e) override;

      private:
        void onEnabled ();
        void onDisabled ();

        void activationCleanup ();

        void checkMode ();
        void switchMode (ModuleMode newMode);

        void confirmQueue ();

        void processActive (const ProcessArgs& args);

        void processIndex (const ProcessArgs& args);
        void processScroll (const ProcessArgs& args);
        void processNoteMap (const ProcessArgs& args);
    };

    struct ConductorExternalWidget : Widgets::ModuleWidgetBase<ConductorExternalModule> {
      private:
        Widgets::EmblemWidget* emblemWidget = nullptr;

        LedNumberDisplay* selectionDisplay = nullptr;
        LedNumberDisplay* queueDisplay = nullptr;

      public:
        ConductorExternalWidget (ConductorExternalModule* module);

      protected:
        void initializeWidget () override;

        void onChangeEmblem (EmblemId emblemId) override;
        void appendContextMenu (rack::ui::Menu* menu) override;

        void step () override;
    };
}