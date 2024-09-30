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
#include "../UI/ImageWidget.hpp"
#include "../UI/WidgetBase.hpp"

namespace OuroborosModules::Modules::Bernoulli {
    struct BernoulliGate {
        rack::dsp::SchmittTrigger schmittTrigger;

        bool modeToggle;
        bool modeLatch;

        std::function<float ()> probabilityFunc;
        bool selectedOutput;

        BernoulliGate () { }
        BernoulliGate (std::function<float ()> probabilityFunc);

        rack::math::Vec process (float gateInput);

        json_t* dataToJson () const;
        bool dataFromJson (json_t* rootJ);
    };

    struct BernoulliModule : ModuleBase {
        static constexpr int GatesCount = 8;

        enum ParamId {
            ENUMS (PARAM_PROBABILITY, GatesCount),
            ENUMS (PARAM_PROBABILITY_CV, GatesCount),
            ENUMS (PARAM_MODE, GatesCount),

            PARAMS_LEN
        };
        enum InputId {
            ENUMS (INPUT_TRIGGER, GatesCount),
            ENUMS (INPUT_PROBABILITY_CV, GatesCount),

            INPUTS_LEN
        };
        enum OutputId {
            ENUMS (OUTPUT_A, GatesCount),
            ENUMS (OUTPUT_B, GatesCount),

            OUTPUTS_LEN
        };
        enum LightId {
            ENUMS (LIGHT_STATE_A, GatesCount),
            ENUMS (LIGHT_STATE_B, GatesCount),

            LIGHTS_LEN
        };

        BernoulliGate bernoulliGates [GatesCount];

        rack::dsp::ClockDivider clockLights;

        BernoulliModule ();

        json_t* dataToJson () override;
        void dataFromJson (json_t* rootJ) override;

        void process (const ProcessArgs& args) override;
        void onReset (const ResetEvent& e) override;
    };

    struct BernoulliWidget : Widgets::ModuleWidgetBase<BernoulliWidget, BernoulliModule> {
      private:
        Widgets::ImageWidget* emblemWidget = nullptr;

      public:
        BernoulliWidget (BernoulliModule* module);

      protected:
        void initializeWidget () override;

        void updateEmblem (ThemeId themeId, EmblemId emblemId);
        void onChangeTheme (ThemeId themeId) override;
        void onChangeEmblem (EmblemId emblemId) override;
    };
}