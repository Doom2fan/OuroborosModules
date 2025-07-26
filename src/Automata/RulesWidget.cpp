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

#include <array>
#include <fmt/format.h>

namespace OuroborosModules::Modules::Automata {
    auto getTitleFont () {
        return APP->window->loadFont (rack::asset::plugin (pluginInstance, "res/fonts/Inconsolata-Bold.ttf"));
    }
    auto getLabelFont () {
        return APP->window->loadFont (rack::asset::plugin (pluginInstance, "res/fonts/Inconsolata-Bold.ttf"));
    }

    struct AutomataRulesWidgetButton : rack_themer::ThemedWidgetBase<rack::widget::Widget> {
      protected:
        AutomataWidget* panelWidget = nullptr;

      public:
        AutomataRulesWidgetButton (rack::math::Vec size, AutomataWidget* panelWidget) {
            this->panelWidget = panelWidget;
            box.size = size;
        }

        virtual void drawButton (const DrawArgs& args) = 0;
        virtual void onPressed () = 0;

        void draw (const DrawArgs& args) override {
            _ThemedWidgetBase::draw (args);

            if (panelWidget->getDisplayLayer () == 0)
                drawButton (args);
        }
        void drawLayer (const DrawArgs& args, int layer) override {
            _ThemedWidgetBase::drawLayer (args, layer);

            if (layer == panelWidget->getDisplayLayer ())
                drawButton (args);
        }

        void onButton (const rack::event::Button& e) override {
            if (e.button == GLFW_MOUSE_BUTTON_LEFT)
                e.consume (this);
        }

        void onDragDrop (const rack::event::DragDrop& e) override {
            _ThemedWidgetBase::onDragDrop (e);

            if (e.origin != this || !isVisible () || !getParent ()->isVisible ())
                return;

            onPressed ();
            e.consume (this);
        }
    };

    struct AutomataRulesWidgetBitButton : AutomataRulesWidgetButton {
      private:
        AutomataRules* rulesPtr;
        uint8_t index;
        bool survivalBit;
        std::string text;

        bool getFlag (int index) {
            return !survivalBit ? rulesPtr->getBirthFlag (index) : rulesPtr->getSurvivalFlag (index);
        }

        void setFlag (int index, bool set) {
            if (!survivalBit)
                rulesPtr->setBirthFlag (index, set);
            else
                rulesPtr->setSurvivalFlag (index, set);
        }

        static auto getFont () { return getLabelFont (); }

      public:
        AutomataRulesWidgetBitButton (AutomataWidget* panelWidget, AutomataRules& rules, uint8_t idx, bool survival)
            : AutomataRulesWidgetButton (rack::math::Vec (), panelWidget) {
            rulesPtr = &rules;
            index = idx;
            survivalBit = survival;

            text = fmt::format (FMT_STRING ("{}"), index);
        }

        void drawButton (const DrawArgs& args) override {
            // Background
            nvgBeginPath (args.vg);
            nvgRect (args.vg, 0, 0, VEC_ARGS (box.size));
            nvgFillColor (args.vg, nvgRGB (64, 64, 64));
            nvgFill (args.vg);

            // Check
            if (getFlag (index)) {
                auto checkSize = box.size * .75f;

                nvgBeginPath (args.vg);
                nvgRect (args.vg, VEC_ARGS (box.size / 2 - checkSize / 2), VEC_ARGS (checkSize));
                nvgFillColor (args.vg, nvgRGBA (255, 255, 255, 64));
                nvgFill (args.vg);
            }

            // Text
            nvgFontFaceId (args.vg, getFont ()->handle);
            nvgFontSize (args.vg, 12);
            nvgTextLetterSpacing (args.vg, 0);

            nvgFillColor (args.vg, rack::color::WHITE);
            nvgTextAlign (args.vg, NVG_ALIGN_MIDDLE | NVG_ALIGN_CENTER);
            nvgText (args.vg, VEC_ARGS (box.size / 2.), &(*text.begin ()), &(*text.end ()));
        }

        void onPressed () override {
            setFlag (index, !getFlag (index));
        }
    };

    struct AutomataRulesWidgetCloseButton : AutomataRulesWidgetButton {
      private:
        bool accept;

        static auto getFont () { return getLabelFont (); }
        std::string_view getText () { return accept ? "Accept" : "Cancel"; }

      public:
        AutomataRulesWidgetCloseButton (rack::math::Vec size, AutomataWidget* panelWidget, bool accept)
            : AutomataRulesWidgetButton (size, panelWidget) {
            this->accept = accept;
        }

        void drawButton (const DrawArgs& args) override {
            // Background
            nvgBeginPath (args.vg);
            nvgRect (args.vg, 0, 0, VEC_ARGS (box.size));
            nvgFillColor (args.vg, nvgRGB (64, 64, 64));
            nvgFill (args.vg);

            // Text
            nvgFontFaceId (args.vg, getFont ()->handle);
            nvgFontSize (args.vg, 12);
            nvgTextLetterSpacing (args.vg, 0);

            std::string_view textStr = getText ();
            nvgFillColor (args.vg, rack::color::WHITE);
            nvgTextAlign (args.vg, NVG_ALIGN_MIDDLE | NVG_ALIGN_CENTER);
            nvgText (args.vg, VEC_ARGS (box.size / 2.), textStr.begin (), textStr.end ());
        }

        void onPressed () override {
            panelWidget->getRulesWidget ()->close (accept);
        }
    };

    struct AutomataRulesWidgetBG : rack_themer::ThemedWidgetBase<rack::widget::Widget> {
      private:
        static constexpr std::string_view TitleText = "Ruleset editing";
        static constexpr std::string_view LabelString = "Rule string:";
        static constexpr std::string_view LabelBirth = "Birth (B):";
        static constexpr std::string_view LabelSurvival = "Survival (S):";

        static constexpr float ContentsBaseY = 30;

        AutomataWidget* panelWidget = nullptr;
        std::array<AutomataRulesWidgetBitButton*, NeighborsCount> birthBitButtons;
        std::array<AutomataRulesWidgetBitButton*, NeighborsCount> survivalBitButtons;

      public:
        AutomataRulesWidgetBG (rack::math::Vec size, AutomataWidget* panelWidget, AutomataRules& rules) {
            using rack::math::Vec;
            using Widgets::createWidget;

            this->panelWidget = panelWidget;
            box.size = size;

            for (int i = 0; i < NeighborsCount; i++) {
                birthBitButtons [i] = createWidget<AutomataRulesWidgetBitButton> (Vec (), panelWidget, rules, i, false);
                addChild (birthBitButtons [i]);

                survivalBitButtons [i] = createWidget<AutomataRulesWidgetBitButton> (Vec (), panelWidget, rules, i, true);
                addChild (survivalBitButtons [i]);
            }
        }

        void doDraw (const DrawArgs& args, int layer) {
            auto drawThisLayer = layer == panelWidget->getDisplayLayer ();

            // Background
            if (drawThisLayer) {
                nvgBeginPath (args.vg);
                nvgRoundedRect (args.vg, 0, 0, VEC_ARGS (box.size), 2);
                nvgFillColor (args.vg, nvgRGBA (0, 0, 0, 0x7F));
                nvgFill (args.vg);
            }

            if (layer == 0)
                _ThemedWidgetBase::draw (args);
            else
                _ThemedWidgetBase::drawLayer (args, layer);

            if (!drawThisLayer)
                return;

            // Title
            auto titleFont = getTitleFont ();
            nvgFontFaceId (args.vg, titleFont->handle);
            nvgFontSize (args.vg, 16);
            nvgTextLetterSpacing (args.vg, 0);

            nvgFillColor (args.vg, rack::color::WHITE);
            nvgTextAlign (args.vg, NVG_ALIGN_TOP | NVG_ALIGN_CENTER);
            nvgText (args.vg, box.size.x / 2., 2, TitleText.begin (), TitleText.end ());

            // Labels
            auto labelFont = getLabelFont ();

            nvgFontFaceId (args.vg, labelFont->handle);
            nvgFontSize (args.vg, 12);
            nvgTextLetterSpacing (args.vg, 0);

            nvgFillColor (args.vg, rack::color::WHITE);
            nvgTextAlign (args.vg, NVG_ALIGN_BASELINE | NVG_ALIGN_LEFT);

            nvgText (args.vg, 5, ContentsBaseY + 0, LabelString.begin (), LabelString.end ());
            auto rulesString = panelWidget->getRulesWidget ()->getRulesString ();
            nvgText (args.vg, 15, ContentsBaseY + 15, rulesString.begin (), rulesString.end ());

            nvgText (args.vg, 5, ContentsBaseY + 30, LabelBirth.begin (), LabelBirth.end ());
            nvgText (args.vg, 5, ContentsBaseY + 65, LabelSurvival.begin (), LabelSurvival.end ());
        }

        void draw (const DrawArgs& args) override {
            using rack::math::Vec;

            auto buttonSize = 15;
            auto buttonSpacing = 4;

            auto buttonBaseX = 8;
            auto birthButtonY = ContentsBaseY + 30 + 6;
            auto survivalButtonY = ContentsBaseY + 65 + 6;

            for (int i = 0; i < NeighborsCount; i++) {
                auto birthButton = birthBitButtons [i];
                birthButton->box.size = Vec (buttonSize);
                birthButton->box.pos = Vec (buttonBaseX + i * (buttonSize + buttonSpacing), birthButtonY);

                auto survivalButton = survivalBitButtons [i];
                survivalButton->box.size = Vec (buttonSize);
                survivalButton->box.pos = Vec (buttonBaseX + i * (buttonSize + buttonSpacing), survivalButtonY);
            }

            doDraw (args, 0);
        }

        void drawLayer (const DrawArgs& args, int layer) override { doDraw (args, layer); }
    };

    AutomataRulesWidget::AutomataRulesWidget (rack::math::Vec size, AutomataWidget* panelWidget) {
        using rack::math::Vec;
        using Widgets::createWidget;

        this->panelWidget = panelWidget;
        box.size = size;

        //addChild (Widgets::createWidget<AutomataRulesWidgetBG> (Vec (), size / 2., panelWidget));
        addChild (Widgets::createWidget<AutomataRulesWidgetBG> (Vec (BoardMargin), size - BoardMargin * 2, panelWidget, rules));

        auto closeButtonSize = Vec (40, 18);
        auto closeButtonBasePos = box.size - 15;
        addChild (createWidget<AutomataRulesWidgetCloseButton> (closeButtonBasePos - closeButtonSize, closeButtonSize, panelWidget, false));
        closeButtonBasePos.x -= closeButtonSize.x + 5;
        addChild (createWidget<AutomataRulesWidgetCloseButton> (closeButtonBasePos - closeButtonSize, closeButtonSize, panelWidget, true));

        hide ();
        active = false;
    }

    void AutomataRulesWidget::updateRuleString (bool forced) {
        if (!forced && ruleString_Base == rules)
            return;

        ruleString_Base = rules;
        ruleString = ruleString_Base.getRuleString ();
    }

    void AutomataRulesWidget::open () {
        assert (!active);
        if (active)
            return;

        auto module = panelWidget->getAutomata ();
        if (module == nullptr)
            return;

        rules = module->getLifeBoard ().getRules ();
        updateRuleString (true);

        show ();
        active = true;
    }

    void AutomataRulesWidget::close (bool accept) {
        if (!active)
            return;

        if (accept) {
            if (auto module = panelWidget->getAutomata ())
                APP->history->push (new HistoryChangeRules (module, rules));
        }

        hide ();
        active = false;
    }
}