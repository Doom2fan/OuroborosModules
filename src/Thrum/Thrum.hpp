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

#include "../ModuleBase.hpp"
#include "../PluginDef.hpp"
#include "../DSP/ClockDivider.hpp"
#include "../UI/CommonWidgets.hpp"
#include "../UI/WidgetBase.hpp"
#include "../Utils.hpp"

#include "DrumCore.hpp"

namespace OuroborosModules::Modules::Thrum {
    struct ThrumModule : ModuleBase {
        enum ParamIds {
            PARAM_FREQ,

            PARAM_EXCITE_SINE_LEVEL,
            PARAM_EXCITE_NOISE_LEVEL,

            PARAM_EXCITE_NOISE_BLEED_LEVEL,

            PARAM_FEEDBACK,
            PARAM_DAMPING,

            PARAM_ALLPASS_FREQ,
            PARAM_ALLPASS_GAIN,

            PARAM_PITCHBEND_DECAY,
            PARAM_PITCHBEND_DEPTH,

            PARAM_SNARE_LEVEL,
            PARAM_SNARE_FILTER_FREQ,
            PARAM_SNARE_FILTER_TYPE,
            PARAM_SNARE_FALL_TIME,

            PARAM_VELOCITY,

            PARAM_RESET_ON_HIT,

            NUM_PARAMS
        };
        enum InputIds {
            INPUT_TRIGGER,
            INPUT_VELOCITY,

            NUM_INPUTS
        };
        enum OutputIds {
            OUTPUT_SIGNAL,

            NUM_OUTPUTS
        };
        enum LightIds {
            NUM_LIGHTS
        };

        rack::dsp::SchmittTrigger hitTrigger [Constants::MaxPolyphony];
        DrumCore drumCores [Constants::MaxPolyphony];

        DSP::ClockDivider clockParams;

        ThrumModule ();

        void process (const ProcessArgs& args) override;

        void onSampleRateChange (const SampleRateChangeEvent& e) override;
    };

    struct ThrumWidget : Widgets::ModuleWidgetBase<ThrumModule> {
      private:
        Widgets::EmblemWidget* emblemWidget = nullptr;

      public:
        ThrumWidget (ThrumModule* module);

      protected:
        void initializeWidget () override;

        void onChangeEmblem (EmblemId emblemId) override;
    };
}