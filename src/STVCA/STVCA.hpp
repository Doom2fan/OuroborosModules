/*
 *  OuroborosModules
 *  Copyright (C) 2024-2025 Chronos "phantombeta" Ouroboros
 *  Copyright (C) 2016-2023 VCV
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
#include "../Utils.hpp"

namespace OuroborosModules::Modules::STVCA {
    struct STVCAModule : ModuleBase {
        enum ParamId {
            PARAM_LEVEL,
            PARAM_EXP,

            PARAMS_LEN
        };
        enum InputId {
            INPUT_LEFT,
            INPUT_RIGHT,
            INPUT_CV,

            INPUTS_LEN
        };
        enum OutputId {
            OUTPUT_LEFT,
            OUTPUT_RIGHT,

            OUTPUTS_LEN
        };
        enum LightId {
            LIGHTS_LEN
        };

        bool displayColorUseDefault = true;
        RGBColor displayColor = RGBColor ();

        int lastChannels = 1;
        float lastGains [16] = { };

        STVCAModule ();

        json_t* dataToJson () override;
        void dataFromJson (json_t* rootJ) override;

        void process (const ProcessArgs& args) override;
        void onReset (const ResetEvent& e) override;
    };

    struct STVCAWidget : Widgets::ModuleWidgetBase<STVCAWidget, STVCAModule> {
      private:
        Widgets::EmblemWidget* emblemWidget = nullptr;

      public:
        STVCAWidget (STVCAModule* module);

      protected:
        void initializeWidget () override;

        void onChangeEmblem (EmblemId emblemId) override;
        void createLocalStyleMenu (rack::ui::Menu* menu) override;
        void createPluginSettingsMenu (rack::ui::Menu* menu) override;
    };
}