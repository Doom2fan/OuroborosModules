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

#include "Common.hpp"

#include "../DSP/ClockDivider.hpp"
#include "../PluginDef.hpp"
#include "../UI/CommonWidgets.hpp"
#include "../UI/WidgetBase.hpp"

namespace OuroborosModules::Modules::Conductor {
    struct ConductorModule : ConductorCore {
        struct ClockHandler {
            static constexpr uint32_t MaxDelay = 50;

          private:
            bool ignoreFirstClock = false;
            uint32_t delayTime = 0;

            float resetPhasor = 0.;

            uint32_t clockDelayLineIndex = 0;
            float clockDelayLine [MaxDelay + 1] = { };

            bool clockHigh = false;
            rack::dsp::SchmittTrigger clockTrigger;

          public:
            void setParams (bool ignoreFirstClock, uint32_t delay);

            void resetPulse ();
            bool processClock (const ProcessArgs& args, float clockInput);
        };

        enum ParamIds {
            PARAM_SEQ_MAX_PATTERNS,
            PARAM_SEQ_MAX_CV,

            PARAM_PATTERN_COUNT,

            PARAM_RESET_PATTERN,
            PARAM_RESET_PATTERN_BUTTON,

            PARAM_MANUAL_RESET_BUTTON,
            PARAM_MANUAL_ADVANCE_BUTTON,

            PARAM_MANUAL_SET,
            PARAM_MANUAL_SET_BUTTON,

            PARAM_PATTERN_OFFSET_VOLTAGE,

            PARAM_CLOCK_DELAY,

            PARAMS_LEN
        };
        enum InputIds {
            INPUT_CLOCK,
            INPUT_ADVANCE,
            INPUT_RESET,

            INPUTS_LEN
        };
        enum OutputIds {
            OUTPUT_CLOCK,
            OUTPUT_RESET,
            OUTPUT_PATTERN,

            OUTPUTS_LEN
        };
        enum LightIds {
            LIGHT_RESET_BUTTON,
            LIGHT_ADVANCE_BUTTON,
            LIGHT_RESET_PATTERN_BUTTON,
            LIGHT_MANUAL_SET_BUTTON,

            LIGHTS_LEN
        };

        // State
        ClockHandler clockHandler;

        int curSequencerMax = 0;
        int curPatternCount = 0;
        int curResetPattern = 0;

        int currentPattern = 0;
        int queuedPattern = QueueCleared;

        bool resetQueuedOn = false;
        bool resetPatternOn = false;
        bool resetIgnoreFirstClock = true;

        float curPatternOffset = 0.f;
        float curMaxCV = 0.f;
        float curPatternCV = 0.f;

        bool dataUpdated = true;

        // Triggers
        rack::dsp::SchmittTrigger advanceTrigger;
        rack::dsp::SchmittTrigger resetTrigger;

        rack::dsp::SchmittTrigger advanceButtonTrigger;
        rack::dsp::SchmittTrigger resetButtonTrigger;
        rack::dsp::SchmittTrigger resetPatternToggleTrigger;
        rack::dsp::SchmittTrigger patternSetButtonTrigger;

        // Pulses
        rack::dsp::PulseGenerator resetPulse;
        rack::dsp::PulseGenerator advanceLightPulse;
        rack::dsp::PulseGenerator resetLightPulse;
        rack::dsp::PulseGenerator manualSetLightPulse;

        // Clock dividers
        DSP::ClockDivider clockLights;

        ConductorModule ();

        json_t* dataToJson () override;
        void dataFromJson (json_t* rootJ) override;

        void process (const ProcessArgs& args) override;

        void requestEnqueue (int newQueue) override { queuePattern (newQueue); }
        void requestDequeue () override { dequeuePattern (); }

        void onReset (const ResetEvent& e) override;

      protected:
        void prepareDataUpdatedEvent (ConductorDataUpdatedEvent& e) override;

      private:
        void changePattern (int newPattern);

        void queuePattern (int newPattern);
        void dequeuePattern ();

        void calculatePatternInfo ();
        void calculatePatternCV ();

        void handleReset ();
        void handleAdvance ();
    };

    struct ConductorWidget : Widgets::ModuleWidgetBase<ConductorModule> {
      private:
        Widgets::EmblemWidget* emblemWidget = nullptr;

      public:
        ConductorWidget (ConductorModule* module);

      protected:
        void initializeWidget () override;

        void onChangeEmblem (EmblemId emblemId) override;
        void appendContextMenu (rack::ui::Menu* menu) override;
    };
}