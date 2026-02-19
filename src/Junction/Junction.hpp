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

#include <array>

namespace OuroborosModules::Modules::Junction {
    struct JunctionModule : ModuleBase {
        static constexpr uint8_t SwitchCount = 8;
        static constexpr uint8_t OutputCount = 2;

        static constexpr int SIMDBankSize = 4;
        static constexpr int SIMDBankCount = static_cast<int> (static_cast<float> (Constants::MaxPolyphony) / SIMDBankSize + .5f);

        struct OutputData {
            uint8_t inputs [SwitchCount];
            uint8_t inputCount = 0;

            void resetInputs () { inputCount = 0; }
            void addInput (uint8_t id) { inputs [inputCount++] = id; }
        };

        enum ParamIds {
            ENUMS (PARAM_SWITCH, SwitchCount),

            PARAMS_LEN
        };
        enum InputIds {
            ENUMS (INPUT_SIGNAL, SwitchCount),

            INPUTS_LEN
        };
        enum OutputIds {
            ENUMS (OUTPUT_SIGNAL, OutputCount),

            OUTPUTS_LEN
        };
        enum LightIds {
            LIGHTS_LEN
        };

        bool polyOnDemand = false;
        bool clampWhileSumming = false;

        std::array<OutputData, OutputCount> outputData;
        DSP::ClockDivider clockUpdate;

        JunctionModule ();

        json_t* dataToJson () override;
        void dataFromJson (json_t* rootJ) override;

        void process (const ProcessArgs& args) override;
    };

    struct JunctionWidget : Widgets::ModuleWidgetBase<JunctionModule> {
      private:
        Widgets::EmblemWidget* emblemWidget = nullptr;

      public:
        JunctionWidget (JunctionModule* module);

      protected:
        void initializeWidget () override;

        void onChangeEmblem (EmblemId emblemId) override;
        void appendContextMenu (rack::ui::Menu* menu) override;
    };
}