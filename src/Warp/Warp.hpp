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
#include "../DSP/Filters.hpp"
#include "../DSP/HilbertTransform.h"
#include "../DSP/Resamplers.hpp"
#include "../ModuleBase.hpp"
#include "../PluginDef.hpp"
#include "../UI/CommonWidgets.hpp"
#include "../UI/WidgetBase.hpp"

namespace OuroborosModules::Modules::Warp {
    struct WarpModule : ModuleBase {
        enum ParamIds {
            PARAM_AMOUNT,
            PARAM_BIAS,

            PARAM_AMOUNT_CV_ATTEN,
            PARAM_BIAS_CV_ATTEN,

            PARAM_OVERSAMPLE,

            PARAMS_LEN
        };
        enum InputIds {
            INPUT_SIGNAL,
            INPUT_MODULATOR,

            INPUT_AMOUNT_CV,
            INPUT_BIAS_CV,

            INPUTS_LEN
        };
        enum OutputIds {
            OUTPUT_SIGNAL,

            OUTPUTS_LEN
        };
        enum LightIds {
            LIGHTS_LEN
        };

        static constexpr int MaxOversample = 16;
        static constexpr int DefaultOversampleRate = 4;

        static constexpr float MaxBias = 5;

        // State
        uint32_t curSampleRate;
        uint32_t oversampleRate;

        // Oversampling
        DSP::OptimizedHalfBandInterpolator<float> signalUpsampler [Constants::MaxPolyphony];
        DSP::OptimizedHalfBandInterpolator<float> upsamplerFilter [Constants::MaxPolyphony];
        DSP::OptimizedHalfBandDecimator<float> downsamplerFilter [Constants::MaxPolyphony];

        // Filters
        DSP::HilbertTransform hilbertTransformSignal [Constants::MaxPolyphony];
        DSP::HilbertTransform hilbertTransformModulator [Constants::MaxPolyphony];
        DSP::DCBlocker<float> dcBlocker [Constants::MaxPolyphony];

        // Clock dividers
        DSP::ClockDivider clockOversample;

        WarpModule ();

        void process (const ProcessArgs& args) override;

        void onSampleRateChange (const SampleRateChangeEvent& e) override;

      private:
        void processChannel (int channel);

        void setOversampleRate (uint32_t newOversampleRate);
        void updateSampleRate (uint32_t newSampleRate);
    };

    struct WarpWidget : Widgets::ModuleWidgetBase<WarpModule> {
      private:
        Widgets::EmblemWidget* emblemWidget = nullptr;

      public:
        WarpWidget (WarpModule* module);

      protected:
        void initializeWidget () override;

        void onChangeEmblem (EmblemId emblemId) override;
        void appendContextMenu (rack::ui::Menu* menu) override;
    };
}