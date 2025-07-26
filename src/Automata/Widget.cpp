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

#include "Automata.hpp"

#include "../UI/WidgetUtils.hpp"

#include <fmt/format.h>

namespace OuroborosModules::Modules::Automata {
    AutomataWidget::AutomataWidget (AutomataModule* module) { constructor (module, "panels/Automata"); }

    void AutomataWidget::initializeWidget () {
        using rack::createInputCentered;
        using rack::createLightParamCentered;
        using rack::createOutputCentered;
        using rack::createParamCentered;
        using rack::componentlibrary::WhiteLight;
        using rack::math::Vec;
        using Widgets::CableJackInput;
        using Widgets::CableJackOutput;
        using Widgets::createLightCentered;
        using Widgets::createWidget;
        using Widgets::EmblemWidget;
        using Widgets::MetalKnobSmall;
        using Widgets::ResizableVCVLight;
        using Widgets::ScrewWidget;
        using Widgets::TrimmerKnob;

        using LightButton = Widgets::LightButton<>;

        addChild (createWidget<ScrewWidget> (Vec ()));
        addChild (createWidget<ScrewWidget> (Vec (box.size.x, RACK_GRID_HEIGHT).minus (Vec (RACK_GRID_WIDTH))));

        emblemWidget = new Widgets::EmblemWidget (curEmblem, findNamed ("widgetLogo", Vec ()));
        addChild (emblemWidget);

        // Inputs
        addInput (createInputCentered<CableJackInput> (findNamed ("input_Clock", Vec ()), moduleT, AutomataModule::INPUT_CLOCK));
        addInput (createInputCentered<CableJackInput> (findNamed ("input_Reset", Vec ()), moduleT, AutomataModule::INPUT_RESET));

        addInput (createInputCentered<CableJackInput> (findNamed ("input_RandomDensity", Vec ()), moduleT, AutomataModule::INPUT_RANDOM_DENSITY_CV));
        addInput (createInputCentered<CableJackInput> (findNamed ("input_Randomize", Vec ()), moduleT, AutomataModule::INPUT_RANDOMIZE));

        addInput (createInputCentered<CableJackInput> (findNamed ("input_LengthEnable", Vec ()), moduleT, AutomataModule::INPUT_LENGTH_ENABLE));
        addInput (createInputCentered<CableJackInput> (findNamed ("input_LengthCV", Vec ()), moduleT, AutomataModule::INPUT_LENGTH_CV));

        // Outputs
        forEachMatched ("output_Trigger(\\d+)", [&] (std::vector<std::string> captures, Vec pos) {
            auto i = stoi (captures [0]) - 1;
            if (i < 0 || i > TriggerCount)
                return LOG_WARN (FMT_STRING ("Automata panel has invalid trigger output #{}"), i);
            addOutput (createOutputCentered<CableJackOutput> (pos, moduleT, AutomataModule::OUTPUT_TRIGGER + i));
        });
        addOutput (createOutputCentered<CableJackOutput> (findNamed ("output_EOC", Vec ()), moduleT, AutomataModule::OUTPUT_EOC));

        // Params
        addChild (createLightParamCentered<LightButton> (findNamed ("param_StepButton", Vec ()), moduleT, AutomataModule::PARAM_STEP_BUTTON, AutomataModule::LIGHT_STEP_BUTTON));
        addChild (createLightParamCentered<LightButton> (findNamed ("param_ResetButton", Vec ()), moduleT, AutomataModule::PARAM_RESET_BUTTON, AutomataModule::LIGHT_RESET_BUTTON));

        addChild (createLightParamCentered<LightButton> (findNamed ("param_RandomButton", Vec ()), moduleT, AutomataModule::PARAM_RANDOMIZE_BUTTON, AutomataModule::LIGHT_RANDOMIZE_BUTTON));
        addChild (createParamCentered<MetalKnobSmall> (findNamed ("param_RandomDensity", Vec ()), moduleT, AutomataModule::PARAM_RANDOM_DENSITY));
        addChild (createParamCentered<TrimmerKnob> (findNamed ("param_RandomDensityCVAtten", Vec ()), moduleT, AutomataModule::PARAM_RANDOM_DENSITY_CV_ATTENUVERTER));

        addChild (createLightParamCentered<LightButton> (findNamed ("param_LengthButton", Vec ()), moduleT, AutomataModule::PARAM_LENGTH_BUTTON, AutomataModule::LIGHT_LENGTH_BUTTON));
        addChild (createParamCentered<MetalKnobSmall> (findNamed ("param_Length", Vec ()), moduleT, AutomataModule::PARAM_LENGTH));
        addChild (createParamCentered<TrimmerKnob> (findNamed ("param_LengthCVAtten", Vec ()), moduleT, AutomataModule::PARAM_LENGTH_CV_ATTENUVERTER));

        addChild (createParamCentered<MetalKnobSmall> (findNamed ("param_ModeSelect", Vec ()), moduleT, AutomataModule::PARAM_MODE_SELECT));

        auto displayBox = findNamedBox ("widget_Board", rack::math::Rect ());
        auto boardWidget = createWidget<AutomataBoardWidget> (displayBox.pos, displayBox.size, this);
        addChild (boardWidget);

        rulesWidget = createWidget<AutomataRulesWidget> (Vec (), displayBox.size, this);
        boardWidget->addChild (rulesWidget);
    }

    void AutomataWidget::onChangeEmblem (EmblemId emblemId) {
        _WidgetBase::onChangeEmblem (emblemId);
        emblemWidget->setEmblem (emblemId);
    }

    void AutomataWidget::appendContextMenu (rack::ui::Menu* menu) {
        using rack::createBoolPtrMenuItem;
        using rack::createMenuItem;
        using rack::createMenuLabel;
        using rack::createSubmenuItem;
        using rack::ui::Menu;

        _WidgetBase::appendContextMenu (menu);

        menu->addChild (new rack::ui::MenuSeparator);
        menu->addChild (createMenuLabel ("Actions"));
        menu->addChild (createMenuItem ("Clear seed", "", [=] {
            moduleT->addEditCommand<EditCommand_Clear> (moduleT->lifeBoard.getBoard (), AutomataCell::FLAG_SeedSet);
        }));

        menu->addChild (new rack::ui::MenuSeparator);
        menu->addChild (createMenuLabel ("Options"));
        menu->addChild (createSubmenuItem ("Rule options", "", [=] (Menu* menu) { generateRulesContextMenu (menu); }));
        menu->addChild (createSubmenuItem ("Trigger set options", "", [=] (Menu* menu) {
            for (int i = 0; i < TriggerCount; i++) {
                menu->addChild (createSubmenuItem (
                    fmt::format (FMT_STRING ("Trigger set #{}"), i + 1), "", [=] (Menu* menu) {
                        generateTriggerContextMenu (menu, i);
                    }
                ));
            }
        }));

        menu->addChild (new rack::ui::MenuEntry);
        menu->addChild (createBoolPtrMenuItem ("Randomize on manual reset", "", &moduleT->randomizeOnManualReset));
        menu->addChild (createBoolPtrMenuItem ("Randomize on automatic reset", "", &moduleT->randomizeOnAutoReset));
        menu->addChild (createBoolPtrMenuItem ("Momentary length enable input", "", &moduleT->momentaryLengthEnable));
    }

    void AutomataWidget::generateRulesContextMenu (rack::ui::Menu* menu) {
        using rack::createCheckMenuItem;
        using rack::createMenuItem;
        using rack::createMenuLabel;

        menu->addChild (createMenuLabel ("Current rule:"));
        menu->addChild (createMenuLabel (moduleT->lifeBoard.getRules ().getRuleString ()));
        menu->addChild (createMenuItem ("Edit rule", "", [=] { rulesWidget->open (); }));

        menu->addChild (new rack::ui::MenuSeparator);
        menu->addChild (createMenuLabel ("Preset rules"));
        for (auto rulePair : presetRules) {
            auto [name, rules] = rulePair;

            menu->addChild (createCheckMenuItem (
                name, rules.getRuleString (),
                [=] { return rules == moduleT->getLifeBoard ().getRules (); },
                [=] { APP->history->push (new HistoryChangeRules (moduleT, rules)); }
            ));
        }
    }

    struct HistoryModifyTriggerInfo : rack::history::ModuleAction {
      protected:
        uint32_t triggerIndex;

        HistoryModifyTriggerInfo (AutomataModule* module, uint32_t index) {
            moduleId = module->id;
            triggerIndex = index;
        }

        AutomataModule* getModule () { return dynamic_cast<AutomataModule*> (APP->engine->getModule (moduleId)); }

        AutomataTriggerInfo* getTriggerInfo () {
            auto module = getModule ();
            return module != nullptr ? &module->getTriggerInfo (triggerIndex) : nullptr;
        }

      public:
        void undo () override = 0;
        void redo () override = 0;
    };

    struct HistoryChangeTriggerCountMode : HistoryModifyTriggerInfo {
      private:
        AutomataTriggerCountMode oldMode;
        AutomataTriggerCountMode newMode;

      public:
        HistoryChangeTriggerCountMode (AutomataModule* module, uint32_t index, AutomataTriggerCountMode newMode)
            : HistoryModifyTriggerInfo (module, index) {
            assert (index < TriggerCount);

            auto& triggerInfo = module->getTriggerInfo (triggerIndex);
            oldMode = triggerInfo.countMode;
            triggerInfo.countMode = this->newMode = newMode;

            name = fmt::format (FMT_STRING ("change automata trigger set #{}'s cell detection mode"), triggerIndex + 1);
        }

        void undo () override {
            auto triggerInfo = getTriggerInfo ();

            if (triggerInfo == nullptr)
                return;

            triggerInfo->countMode = oldMode;
        }

        void redo () override {
            auto triggerInfo = getTriggerInfo ();

            if (triggerInfo == nullptr)
                return;

            triggerInfo->countMode = newMode;
        }
    };

    struct HistoryChangeTriggerOutputMode : HistoryModifyTriggerInfo {
      private:
        AutomataTriggerOutputMode oldMode;
        AutomataTriggerOutputMode newMode;

      public:
        HistoryChangeTriggerOutputMode (AutomataModule* module, uint32_t index, AutomataTriggerOutputMode newMode)
            : HistoryModifyTriggerInfo (module, index) {
            assert (index < TriggerCount);

            auto& triggerInfo = module->getTriggerInfo (triggerIndex);
            oldMode = triggerInfo.outputMode;
            triggerInfo.outputMode = this->newMode = newMode;

            name = fmt::format (FMT_STRING ("change automata trigger set #{}'s output mode"), triggerIndex + 1);
        }

        void undo () override {
            auto triggerInfo = getTriggerInfo ();

            if (triggerInfo == nullptr)
                return;

            triggerInfo->outputMode = oldMode;
        }

        void redo () override {
            auto triggerInfo = getTriggerInfo ();

            if (triggerInfo == nullptr)
                return;

            triggerInfo->outputMode = newMode;
        }
    };

    void AutomataWidget::generateTriggerContextMenu (rack::ui::Menu* menu, int i) {
        using rack::createCheckMenuItem;
        using rack::createMenuItem;
        using rack::createSubmenuItem;
        using rack::ui::Menu;

        menu->addChild (createSubmenuItem ("Cell detection mode", "", [=] (Menu* menu) {
            auto createFunc = [=] (std::string name, AutomataTriggerCountMode mode) {
                menu->addChild (createCheckMenuItem (name, "",
                    [=] { return moduleT->getTriggerInfo (i).countMode == mode; },
                    [=] { APP->history->push (new HistoryChangeTriggerCountMode (moduleT, i, mode)); }
                ));
            };

            createFunc ("Living cells", AutomataTriggerCountMode::IsAlive);
            createFunc ("Newborn cells", AutomataTriggerCountMode::Newborn);
        }));

        menu->addChild (createSubmenuItem ("Output mode", "", [=] (Menu* menu) {
            auto createFunc = [=] (std::string name, AutomataTriggerOutputMode mode) {
                menu->addChild (createCheckMenuItem (name, "",
                    [=] { return moduleT->getTriggerInfo (i).outputMode == mode; },
                    [=] { APP->history->push (new HistoryChangeTriggerOutputMode (moduleT, i, mode)); }
                ));
            };

            createFunc ("Trigger on any cells active", AutomataTriggerOutputMode::Trigger);
            createFunc ("Percentage of total cells active", AutomataTriggerOutputMode::Percentage);
        }));

        menu->addChild (createMenuItem ("Clear trigger locations", "", [=] {
            moduleT->addEditCommand<EditCommand_Clear> (moduleT->lifeBoard.getBoard (), cellFromTriggerIndex (i));
        }));
    }
}