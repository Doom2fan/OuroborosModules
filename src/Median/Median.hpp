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

#include "../DSP/Filters.hpp"
#include "../ModuleBase.hpp"
#include "../PluginDef.hpp"
#include "../UI/CommonWidgets.hpp"
#include "../UI/WidgetBase.hpp"

namespace OuroborosModules::Modules::Median {
    struct MedianModule : ModuleBase {
        enum ParamId {
            ENUMS (PARAM_VAL_SCALE, 3),
            ENUMS (PARAM_VAL_OFFSET, 3),

            PARAM_OVERSAMPLE,

            PARAMS_LEN
        };
        enum InputId {
            ENUMS (INPUT_VALUES, 3),

            INPUTS_LEN
        };
        enum OutputId {
            OUTPUT_MIN,
            OUTPUT_MID,
            OUTPUT_MAX,

            OUTPUTS_LEN
        };
        enum LightId {
            ENUMS (LIGHT_OUTPUT, 3 * 3),

            LIGHTS_LEN
        };

        enum OutputLightId {
            OUTLIGHT_Min = 3 * 0,
            OUTLIGHT_Mid = 3 * 1,
            OUTLIGHT_Max = 3 * 2,
        };

        static constexpr int SIMDBankSize = 4;
        static constexpr int SIMDBankCount = static_cast<int> (static_cast<float> (Constants::MaxPolyphony) / SIMDBankSize + .5f);
        static constexpr int MaxOversample = 16;

        DSP::UpsampleFilter<rack::simd::float_4> upsamplerFilter [SIMDBankCount] [3] {};
        DSP::DownsampleFilter<rack::simd::float_4> downsamplerFilter [SIMDBankCount] [3] {};
        int oversampleRate = 0;

        rack::dsp::ClockDivider clockOversample;
        rack::dsp::ClockDivider clockLights;

        MedianModule ();

        void process (const ProcessArgs& args) override;

      private:
        rack::simd::float_4 getBank (int inputNum, int currentChannel);
        void setOversampleRate (int newOversampleRate);
    };

    struct MedianWidget : Widgets::ModuleWidgetBase<MedianWidget, MedianModule> {
      private:
        Widgets::EmblemWidget* emblemWidget = nullptr;

      public:
        MedianWidget (MedianModule* module);

      protected:
        void initializeWidget () override;

        void onChangeEmblem (EmblemId emblemId) override;
        void appendContextMenu (rack::ui::Menu* menu) override;
    };
}