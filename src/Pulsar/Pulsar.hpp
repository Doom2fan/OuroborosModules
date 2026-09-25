/*
 *  OuroborosModules
 *  Copyright (C) 2026 Chronos "phantombeta" Ouroboros
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
#include "../DSP/Wavetable.hpp"
#include "../ModuleBase.hpp"
#include "../PluginDef.hpp"
#include "../UI/CommonWidgets.hpp"
#include "../UI/WidgetBase.hpp"
#include "../Utils.hpp"

#include "PulsarEngine.hpp"

namespace OuroborosModules::Modules::Pulsar {
    struct PulsarModule : ModuleBase {
        enum ParamIds {
            // Frequency
            PARAM_FREQUENCY,
            PARAM_FORMANT,
            PARAM_CLUSTER,

            // Wave shape
            PARAM_WAVEINDEX,
            PARAM_WAVESHAPER,

            // Windowing
            PARAM_WINDOWINDEX,
            PARAM_WINDOWSKEW,
            PARAM_WINDOWAMOUNT,

            // Masking parameters
            PARAM_BURSTCOUNT,
            PARAM_RESTCOUNT,

            // Mode switches
            PARAM_MASKINGMODE,
            PARAM_DECOUPLE,

            // CV attenuverters
            PARAM_FORMANT_CV_ATTEN,
            PARAM_CLUSTER_CV_ATTEN,

            PARAM_WAVEINDEX_CV_ATTEN,
            PARAM_WAVESHAPER_CV_ATTEN,

            PARAM_WINDOWINDEX_CV_ATTEN,
            PARAM_WINDOWSKEW_CV_ATTEN,
            PARAM_WINDOWAMOUNT_CV_ATTEN,

            PARAM_BURSTCOUNT_CV_ATTEN,
            PARAM_RESTCOUNT_CV_ATTEN,

            // Settings
            PARAM_OVERSAMPLE,
            PARAM_CHANNEL_COUNT,
            PARAM_OVERLAP_MODE,
            PARAM_EDGE_FACTOR,
            PARAM_FREQUENCY_MODE,

            NUM_PARAMS
        };
        enum InputIds {
            // Frequency CV
            INPUT_VOCT,
            INPUT_FORMANT_CV,
            INPUT_CLUSTER_CV,

            // Wave shape CV
            INPUT_WAVEINDEX_CV,
            INPUT_WAVESHAPER_CV,

            // Windowing CV
            INPUT_WINDOWINDEX_CV,
            INPUT_WINDOWSKEW_CV,
            INPUT_WINDOWAMOUNT_CV,

            // Masking parameters CV
            INPUT_BURSTCOUNT_CV,
            INPUT_RESTCOUNT_CV,

            // Misc
            INPUT_SYNC,

            NUM_INPUTS
        };
        enum OutputIds {
            OUTPUT_MAIN,
            OUTPUT_REST,

            NUM_OUTPUTS
        };
        enum LightIds {
            NUM_LIGHTS
        };

        // State
        float curSampleRate;
        float curSampleTime;

        PulsarMaskingMode maskingMode = PulsarMaskingMode::Invalid;
        bool formantDecoupled = false;
        bool overlapMode = false;
        PulsarFrequencyMode frequencyMode = PulsarFrequencyMode::Audio;

        PulsarEngine engine;
        PulsarParameters pulsarParams;

        // Clock dividers
        DSP::ClockDivider clockOversample;
        DSP::ClockDivider clockParams;

        PulsarModule ();

        json_t* dataToJson () override;
        void dataFromJson (json_t* rootJ) override;

        void initializeModule ();

        void updateOversampleRate ();
        void setMaskingMode (PulsarMaskingMode mode, bool force = false);
        void setFormantDecouple (bool decoupled, bool force = false);
        void setOverlapMode (bool overlap, bool force = false);
        void setFrequencyMode (PulsarFrequencyMode mode, bool force = false);

        uint8_t getChannelCount ();
        void updateParams ();

        void process (const ProcessArgs& args) override;

        void onSampleRateChange (const SampleRateChangeEvent& e) override;
        void onReset (const ResetEvent& e) override;
        void onRandomize (const RandomizeEvent& e) override;
    };

    struct PulsarWidget : Widgets::ModuleWidgetBase<PulsarModule> {
      private:
        Widgets::EmblemWidget* emblemWidget = nullptr;

      public:
        PulsarWidget (PulsarModule* module);

      protected:
        void initializeWidget () override;

        void onChangeEmblem (EmblemId emblemId) override;
        void appendContextMenu (rack::ui::Menu* menu) override;
    };
}