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

#include "Chroma.hpp"

#include "CCM_Common.hpp"

namespace OuroborosModules::Modules::Chroma {
    struct OverlayWindow : rack::widget::TransparentWidget {
      private:
        std::string message;

      public:
        OverlayWindow () {
            hide ();
        }

        void display (std::string message) {
            this->message = message;

            box = parent->box;

            show ();
        }

        void close () {
            hide ();

            message = "";
        }

        void draw (const DrawArgs& args) override {
            using rack::math::Vec;

            TransparentWidget::draw (args);

            auto font = APP->window->loadFont (rack::asset::plugin (pluginInstance, "res/fonts/RobotoCondensed.ttf"));
            if (font == nullptr)
                return;

            auto msgStart = message.c_str ();
            auto msgEnd = msgStart + message.length ();

            // Colors.
            NVGcolor textColor, bgColorFill, bgColorStroke; {
                auto menuTheme = bndGetTheme ()->menuTheme;
                textColor = menuTheme.textColor;
                bgColorFill = menuTheme.innerColor;
                bgColorStroke = menuTheme.outlineColor;
            }

            // Set up the font.
            nvgFontFaceId (args.vg, font->handle);
            nvgFontSize (args.vg, 36);
            nvgTextLetterSpacing (args.vg, 0);
            nvgTextAlign (args.vg, NVG_ALIGN_TOP | NVG_ALIGN_LEFT);

            // Get the text's bounds.
            float msgBounds [4];
            nvgTextBoxBounds (args.vg, 0, 0, box.size.x, msgStart, msgEnd, msgBounds);

            // Calculate the size and position of the window.
            auto size = Vec (msgBounds [2], msgBounds [3]) - Vec (msgBounds [0], msgBounds [1]);
            auto pos = box.size.div (2) - size.div (2);

            // Dim the rack.
            nvgBeginPath (args.vg);
            nvgRect (args.vg, 0, 0, box.size.x, box.size.y);

            nvgFillColor (args.vg, nvgRGBAf (0, 0, 0, .5f));
            nvgFill (args.vg);

            // Draw the background.
            nvgBeginPath (args.vg);
            nvgRoundedRect (args.vg, VEC_ARGS (pos - Vec (8)), VEC_ARGS (size + Vec (8 * 2)), 5);

            nvgStrokeWidth (args.vg, 1);
            nvgStrokeColor (args.vg, bgColorStroke);
            nvgStroke (args.vg);
            nvgFillColor (args.vg, bgColorFill);
            nvgFill (args.vg);

            // Draw the text.
            nvgFillColor (args.vg, textColor);
            nvgTextBox (args.vg, VEC_ARGS (pos), box.size.x, msgStart, msgEnd);
        }
    };

    KeyContainer::KeyContainer (ChromaWidget* moduleWidget) {
        this->moduleWidget = moduleWidget;

        overlayWindow = new OverlayWindow ();
        addChild (overlayWindow);

        if (masterKeyContainer == nullptr)
            masterKeyContainer = this;
    }

    KeyContainer::~KeyContainer () {
        if (moduleWidget != nullptr) {
            moduleWidget->keyContainer = nullptr;
            moduleWidget = nullptr;
        }

        if (masterKeyContainer == this)
            masterKeyContainer = nullptr;
    }

    void KeyContainer::step () {
        Widget::step ();

        if (parent != nullptr)
            box.size = parent->box.size;
    }

    void KeyContainer::displayMessage (std::string message) { overlayWindow->display (message); }
    void KeyContainer::closeMessage () { overlayWindow->close (); }

    bool KeyContainer::checkLearningMode (const rack::event::Base& e) {
        if (moduleWidget == nullptr || moduleWidget->module == nullptr)
            return false;

        if (!moduleWidget->moduleT->colorManager->isLearnMode ())
            return false;

        e.consume (this);
        return true;
    }

    void KeyContainer::onButton (const rack::event::Button& e) {
        checkLearningMode (e);

        if (e.action != GLFW_PRESS || e.button == GLFW_KEY_UNKNOWN)
            return;

        if (moduleWidget == nullptr || moduleWidget->module == nullptr)
            return;

        // Disallow LMB, RMB and MMB.
        if (e.button == GLFW_MOUSE_BUTTON_LEFT ||
            e.button == GLFW_MOUSE_BUTTON_RIGHT ||
            e.button == GLFW_MOUSE_BUTTON_MIDDLE)
            return;

        auto cableKey = CableColorKey (e.button, -1, e.mods & RACK_MOD_MASK);
        if (moduleWidget->moduleT->colorManager->handleKey (cableKey))
            e.consume (this);
    }

    void KeyContainer::onHoverKey (const rack::event::HoverKey& e) {
        checkLearningMode (e);

        if (e.action != GLFW_PRESS)
            return;

        if (moduleWidget == nullptr || moduleWidget->module == nullptr)
            return;

        switch (e.key) {
            case GLFW_KEY_UNKNOWN:
            case GLFW_KEY_LEFT_SHIFT:
            case GLFW_KEY_LEFT_CONTROL:
            case GLFW_KEY_LEFT_ALT:
            case GLFW_KEY_LEFT_SUPER:
            case GLFW_KEY_RIGHT_SHIFT:
            case GLFW_KEY_RIGHT_CONTROL:
            case GLFW_KEY_RIGHT_ALT:
            case GLFW_KEY_RIGHT_SUPER:
                return;
        }

        auto cableKey = CableColorKey (-1, e.key, e.mods & RACK_MOD_MASK);
        if (moduleWidget->moduleT->colorManager->handleKey (cableKey))
            e.consume (this);
    }

    void KeyContainer::onHoverText (const rack::event::HoverText& e) {
        if (checkLearningMode (e))
            return;
    }

    void KeyContainer::onHoverScroll (const rack::event::HoverScroll& e) {
        if (checkLearningMode (e))
            return;
    }
}