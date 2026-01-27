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

#include "../DSP/ClockDivider.hpp"
#include "../ModuleBase.hpp"
#include "../PluginDef.hpp"
#include "../UI/CommonWidgets.hpp"
#include "../UI/WidgetBase.hpp"

namespace OuroborosModules::Modules::ResetHelper {
    struct ResetHelperModule : ModuleBase {
        static constexpr int InputCount = 4;

        enum ParamIds {
            PARAM_MERGE_TIME,
            PARAM_TRIGGER_LENGTH,

            NUM_PARAMS
        };
        enum InputIds {
            ENUMS (INPUT_RESET, InputCount * 2),

            NUM_INPUTS
        };
        enum OutputIds {
            OUTPUT_RESET,

            NUM_OUTPUTS
        };
        enum LightIds {
            ENUMS (LIGHT_INPUT, 2 * InputCount),
            ENUMS (LIGHT_OUTPUT, 2),

            NUM_LIGHTS
        };

        rack::dsp::SchmittTrigger inputTriggers [InputCount] [Constants::MaxPolyphony];

        rack::dsp::PulseGenerator outputPulse [Constants::MaxPolyphony];
        rack::dsp::PulseGenerator mergeWindowPulse [Constants::MaxPolyphony];

        rack::dsp::PulseGenerator inputLightPulses [InputCount];
        rack::dsp::PulseGenerator outputLightPulse;

        DSP::ClockDivider clockLights;

        ResetHelperModule ();

        void process (const ProcessArgs& args) override;
    };

    struct ResetHelperWidget : Widgets::ModuleWidgetBase<ResetHelperModule> {
      private:
        Widgets::EmblemWidget* emblemWidget = nullptr;

      public:
        ResetHelperWidget (ResetHelperModule* module);

      protected:
        void initializeWidget () override;

        void onChangeEmblem (EmblemId emblemId) override;
    };
}