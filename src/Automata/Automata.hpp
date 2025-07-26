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

#include "AutomataCommon.hpp"
#include "AutomataLife.hpp"
#include "EditCommands.hpp"

#include <array>
#include <atomic>

namespace OuroborosModules::Modules::Automata {
    struct HistoryAutomataEditCommand : rack::history::ModuleAction {
        std::shared_ptr<EditCommand> command;

        HistoryAutomataEditCommand (AutomataModule* module, std::shared_ptr<EditCommand> newCommand);
        void undo () override;
        void redo () override;
    };

    struct AutomataModule : ModuleBase {
        friend AutomataWidget;
        friend AutomataBoardWidget;

        enum ParamId {
            PARAM_STEP_BUTTON,
            PARAM_RESET_BUTTON,

            PARAM_RANDOMIZE_BUTTON,
            PARAM_RANDOM_DENSITY,
            PARAM_RANDOM_DENSITY_CV_ATTENUVERTER,

            PARAM_LENGTH_BUTTON,
            PARAM_LENGTH,
            PARAM_LENGTH_CV_ATTENUVERTER,

            PARAM_MODE_SELECT,

            PARAMS_LEN
        };
        enum InputId {
            INPUT_CLOCK,
            INPUT_RESET,

            INPUT_RANDOM_DENSITY_CV,
            INPUT_RANDOMIZE,

            INPUT_LENGTH_ENABLE,
            INPUT_LENGTH_CV,

            INPUTS_LEN
        };
        enum OutputId {
            ENUMS (OUTPUT_TRIGGER, TriggerCount),
            OUTPUT_EOC,

            OUTPUTS_LEN
        };
        enum LightId {
            LIGHT_STEP_BUTTON,
            LIGHT_RESET_BUTTON,

            LIGHT_RANDOMIZE_BUTTON,

            LIGHT_LENGTH_BUTTON,

            LIGHTS_LEN
        };

      private:
        // State
        AutomataMode currentMode;
        bool lengthEnabled;
        int stepCount;

        DSP::ClockDivider clockParams;
        DSP::ClockDivider clockLights;

        // Options
        std::array<AutomataTriggerInfo, TriggerCount> triggerInfo;

        bool randomizeOnManualReset;
        bool randomizeOnAutoReset;
        bool momentaryLengthEnable;

        // Inputs
        rack::dsp::SchmittTrigger stepButtonTrigger;
        rack::dsp::SchmittTrigger clockTrigger;

        rack::dsp::SchmittTrigger resetButtonTrigger;
        rack::dsp::SchmittTrigger resetTrigger;

        rack::dsp::SchmittTrigger randomizeButtonTrigger;
        rack::dsp::SchmittTrigger randomizeTrigger;

        rack::dsp::SchmittTrigger lengthButtonTrigger;
        rack::dsp::SchmittTrigger lengthEnableTrigger;

        // Outputs
        rack::dsp::PulseGenerator resetPulse;
        rack::dsp::PulseGenerator eocPulse;
        rack::dsp::PulseGenerator outputPulses [TriggerCount];
        float outputValues [TriggerCount];

        // Board data
        AutomataLife lifeBoard;

        // Widget communication
        CommandQueue commandQueue;
        std::atomic<bool> updateRulesSignal;
        AutomataRules newRules;

      public:
        AutomataModule ();

        template<class TCommand, typename... TArgs>
        std::shared_ptr<EditCommand> addEditCommand (TArgs &&...args) {
            auto cmd = std::make_shared<TCommand> (std::forward<TArgs> (args) ...);
            cmd->executed = false;
            commandQueue.enqueue (cmd, false);
            APP->history->push (new HistoryAutomataEditCommand (this, cmd));

            return cmd;
        }

        AutomataTriggerInfo& getTriggerInfo (int idx) {
            assert (idx >= 0 && idx < TriggerCount);
            return triggerInfo [idx];
        }
        AutomataLife& getLifeBoard () { return lifeBoard; }

        void enqueueCommand (std::shared_ptr<EditCommand> command, bool isUndo) { commandQueue.enqueue (command, isUndo); }
        void enqueueRulesUpdate (AutomataRules rules) {
            newRules = rules;
            updateRulesSignal.store (true);
        }

        void initialize ();

        void process (const ProcessArgs& args) override;
        void processReset (bool automatic);
        void processRandomize ();
        void processStep (const ProcessArgs& args);
        void processTriggers ();

        void onReset (const ResetEvent& e) override;

        json_t* dataToJson () override;
        void dataFromJson (json_t* rootJ) override;
    };

    struct AutomataWidget : Widgets::ModuleWidgetBase<AutomataModule> {
      private:
        Widgets::EmblemWidget* emblemWidget = nullptr;
        AutomataRulesWidget* rulesWidget = nullptr;

      public:
        AutomataWidget (AutomataModule* module);

        AutomataModule* getAutomata () { return moduleT; }
        AutomataRulesWidget* getRulesWidget () { return rulesWidget; }
        int getDisplayLayer () { return 1; } // TODO: Add options for different layers?

      protected:
        void initializeWidget () override;

        void onChangeEmblem (EmblemId emblemId) override;
        void appendContextMenu (rack::ui::Menu* menu) override;
        void generateRulesContextMenu (rack::ui::Menu* menu);
        void generateTriggerContextMenu (rack::ui::Menu* menu, int i);
    };

    struct AutomataBoardWidget : rack_themer::ThemedWidgetBase<rack::widget::Widget> {
      private:
        AutomataWidget* panelWidget = nullptr;
        AutomataLifeUpdateIndex lifeUpdateIndex = 0;

        // Edit data
        bool editing = false;
        bool editSet = false;
        AutomataCell editMask = AutomataCell::None;
        EditGrid editGrid;
        std::vector<std::shared_ptr<EditCommand>> editCommands;

        // Internal board
        AutomataBoard internalBoard;

        // Cached data
        AutomataRules lastRules;
        std::string ruleString;
        float lastFooterSize = 0;

        void enterEditMode (AutomataCell mask, int cellX, int cellY, bool set);
        void editModeMouseMoved (int cellX, int cellY);
        void exitEditMode ();

      public:
        AutomataBoardWidget (rack::math::Vec size, AutomataWidget* panelWidget);

        void drawBoard (const DrawArgs& args);

        void step () override;
        void draw (const DrawArgs& args) override;
        void drawLayer (const DrawArgs& args, int layer) override;
        void onButton (const rack::event::Button& e) override;
        void onDragHover (const rack::event::DragHover& e) override;
        void onDragEnd (const rack::event::DragEnd& e) override;
    };

    struct AutomataRulesWidget : rack_themer::ThemedWidgetBase<rack::widget::Widget> {
      private:
        AutomataWidget* panelWidget = nullptr;

        bool active;
        AutomataRules rules;

        std::string ruleString;
        AutomataRules ruleString_Base;
        void updateRuleString (bool forced);

      public:
        AutomataRulesWidget (rack::math::Vec size, AutomataWidget* panelWidget);

        void open ();
        void close (bool accept);
        bool isOpen () { return active; }

        const std::string_view getRulesString () {
            updateRuleString (false);
            return ruleString;
        }
    };

    struct HistoryChangeRules : rack::history::ModuleAction {
      protected:
        AutomataRules oldRules;
        AutomataRules newRules;

      public:
        HistoryChangeRules (AutomataModule* module, AutomataRules rules) {
            moduleId = module->id;
            name = "change automata rules";

            oldRules = module->getLifeBoard ().getRules ();
            newRules = rules;
            redo ();
        }

        void undo () override {
            auto module = dynamic_cast<AutomataModule*> (APP->engine->getModule (moduleId));
            if (module == nullptr)
                return;
            module->enqueueRulesUpdate (oldRules);
        }

        void redo () override {
            auto module = dynamic_cast<AutomataModule*> (APP->engine->getModule (moduleId));
            if (module == nullptr)
                return;
            module->enqueueRulesUpdate (newRules);
        }
    };
}