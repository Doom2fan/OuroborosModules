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

#include "../DSP/ClockDivider.hpp"
#include "../DSP/Filters.hpp"
#include "../ModuleBase.hpp"
#include "../PluginDef.hpp"
#include "../UI/CommonWidgets.hpp"
#include "../UI/WidgetBase.hpp"
#include "../Utils.hpp"

namespace OuroborosModules::Modules::Crush {
    struct CrushModule : ModuleBase, SST_NeighborConnectable_V1 {
        static constexpr int SIMDBankSize = 4;
        static constexpr int SIMDBankCount = static_cast<int> (static_cast<float> (Constants::MaxPolyphony) / SIMDBankSize + .5f);

        enum ParamIds {
            PARAM_TARGET_LEVEL,
            PARAM_AMOUNT,

            PARAM_FILTER_TIME,

            PARAM_AMOUNT_CV_ATTEN,

            NUM_PARAMS
        };
        enum InputIds {
            INPUT_SIGNAL,

            INPUT_TARGET_LEVEL,
            INPUT_AMOUNT_CV,

            NUM_INPUTS
        };
        enum OutputIds {
            OUTPUT_SIGNAL,
            OUTPUT_ENVELOPE,

            NUM_OUTPUTS
        };
        enum LightIds {
            NUM_LIGHTS
        };

        uint32_t curSampleRate = 0;

        rack::dsp::TPeakFilter<rack::simd::float_4> peakFilter [SIMDBankCount];
        DSP::DCBlocker<rack::simd::float_4> dcBlocker [SIMDBankCount];

        DSP::ClockDivider clockParams;

        CrushModule ();

        void dataFromJson (json_t* rootJ) override;

        void process (const ProcessArgs& args) override;

        void onSampleRateChange (const SampleRateChangeEvent& e) override;

      private:
        void setPeakFilter ();

        std::optional<std::vector<labeledStereoPort_t>> getPrimaryInputs () override {
            return {{ std::make_pair ("Input", std::make_pair (INPUT_SIGNAL, -1)) }};
        }

        std::optional<std::vector<labeledStereoPort_t>> getPrimaryOutputs () override {
            return {{ std::make_pair ("Output", std::make_pair (OUTPUT_SIGNAL, -1)) }};
        }
    };

    struct CrushWidget : Widgets::ModuleWidgetBase<CrushModule> {
      private:
        Widgets::EmblemWidget* emblemWidget = nullptr;

      public:
        CrushWidget (CrushModule* module);

      protected:
        void initializeWidget () override;

        void onChangeEmblem (EmblemId emblemId) override;
    };
}