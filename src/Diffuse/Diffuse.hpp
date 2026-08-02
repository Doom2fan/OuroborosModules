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

#include "../DSP/SchroederAllpass.hpp"
#include "../ModuleBase.hpp"
#include "../UI/CommonWidgets.hpp"
#include "../UI/WidgetBase.hpp"

namespace OuroborosModules::Modules::Diffuse {
    struct DiffuseModule : ModuleBase {
        enum ParamIds {
            PARAM_FREQ,
            PARAM_GAIN,
            PARAM_MIX,

            PARAM_FREQ_CV_ATTEN,
            PARAM_GAIN_CV_ATTEN,
            PARAM_MIX_CV_ATTEN,

            NUM_PARAMS
        };
        enum InputIds {
            INPUT_SIGNAL,

            INPUT_VOCT,

            INPUT_FREQ_CV,
            INPUT_GAIN_CV,
            INPUT_MIX_CV,

            NUM_INPUTS
        };
        enum OutputIds {
            OUTPUT_WET,
            OUTPUT_MIX,

            NUM_OUTPUTS
        };
        enum LightIds {
            NUM_LIGHTS
        };

        float maxDelayFreq = 0.f;
        DSP::SchroederAllpass<float> allpass [Constants::MaxPolyphony] { };

        DiffuseModule ();

        void process (const ProcessArgs& args) override;
        void onSampleRateChange (const SampleRateChangeEvent& e) override;
        void onUnBypass (const UnBypassEvent& e) override;
    };

    struct DiffuseWidget : Widgets::ModuleWidgetBase<DiffuseModule> {
      private:
        Widgets::EmblemWidget* emblemWidget = nullptr;

      public:
        DiffuseWidget (DiffuseModule* module);

      protected:
        void initializeWidget () override;

        void onChangeEmblem (EmblemId emblemId) override;
    };
}