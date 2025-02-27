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
#include "../UI/CommonWidgets.hpp"
#include "../UI/WidgetBase.hpp"

namespace OuroborosModules::Modules::Junction {
    struct JunctionModule : ModuleBase {
        static constexpr int SwitchCount = 8;

        static constexpr int SIMDBankSize = 4;
        static constexpr int SIMDBankCount = static_cast<int> (static_cast<float> (Constants::MaxPolyphony) / SIMDBankSize + .5f);

        enum ParamIds {
            ENUMS (PARAM_SWITCH, SwitchCount),

            NUM_PARAMS
        };
        enum InputIds {
            ENUMS (INPUT_SIGNAL, SwitchCount),

            NUM_INPUTS
        };
        enum OutputIds {
            OUTPUT_A,
            OUTPUT_B,

            NUM_OUTPUTS
        };
        enum LightIds {
            NUM_LIGHTS
        };

        bool polyOnDemand = false;

        JunctionModule ();

        json_t* dataToJson () override;
        void dataFromJson (json_t* rootJ) override;

        void process (const ProcessArgs& args) override;
    };

    struct JunctionWidget : Widgets::ModuleWidgetBase<JunctionWidget, JunctionModule> {
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