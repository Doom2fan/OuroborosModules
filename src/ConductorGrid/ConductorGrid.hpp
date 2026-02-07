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
    struct ConductorGridModule : ConductorExpander {
        static constexpr int PadCount = 8;

        enum ParamIds {
            PARAM_PAGE_DOWN_BUTTON,
            PARAM_PAGE_UP_BUTTON,
            ENUMS (PARAM_PAD_BUTTON, PadCount),

            NUM_PARAMS
        };
        enum InputIds {
            NUM_INPUTS
        };
        enum OutputIds {
            NUM_OUTPUTS
        };
        enum LightIds {
            LIGHT_PAGE_DOWN_BUTTON,
            LIGHT_PAGE_UP_BUTTON,
            ENUMS (LIGHT_PAD_BUTTON, PadCount),

            NUM_LIGHTS
        };

        // State
        bool enabled = false;

        int patternCount = 0;
        int currentPattern = 0;
        int queuedPattern = 0;

        int pageCount = 0;
        int curPage = 0;

        // Triggers
        rack::dsp::SchmittTrigger pageDownButtonTrigger;
        rack::dsp::SchmittTrigger pageUpButtonTrigger;
        rack::dsp::SchmittTrigger padButtonTriggers [PadCount];

        // Pulses
        rack::dsp::PulseGenerator pageDownLightPulse;
        rack::dsp::PulseGenerator pageUpLightPulse;
        rack::dsp::PulseGenerator padButtonLightPulses [PadCount];

        // Clock dividers
        DSP::ClockDivider clockLights;

        ConductorGridModule ();

        json_t* dataToJson () override;
        void dataFromJson (json_t* rootJ) override;

        void process (const ProcessArgs& args) override;

        void onDataUpdated (const ConductorDataUpdatedEvent& coreData) override;

      private:
        void queueIndex (int index);

        void processActive (const ProcessArgs& args);
    };

    struct ConductorGridWidget : Widgets::ModuleWidgetBase<ConductorGridModule> {
      private:
        Widgets::EmblemWidget* emblemWidget = nullptr;

        LedNumberDisplay* pageDisplay = nullptr;
        LedNumberDisplay* queueDisplay = nullptr;

      public:
        ConductorGridWidget (ConductorGridModule* module);

      protected:
        void initializeWidget () override;

        void onChangeEmblem (EmblemId emblemId) override;

        void step () override;
    };
}