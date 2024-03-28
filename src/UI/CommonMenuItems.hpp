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

#include "../PluginDef.hpp"

namespace OuroborosModules {
namespace UI {
    struct ColorMenuItem : rack::ui::MenuItem {
        NVGcolor color;

        void draw (const DrawArgs& args) override;
    };

    struct SafeMenuItem : rack::ui::MenuItem {
      private:
        static constexpr const char* defaultConfirmText = "Are you sure?";

        std::string confirmText;
        std::string confirmButtonText;
        std::function<void ()> action;
        bool alwaysConsume;

      public:
        SafeMenuItem (
            std::string text,
            std::string confirmButtonText,
            std::function<void ()> action,
            bool alwaysConsume = false,
            std::string confirmText = defaultConfirmText
        ) : confirmText (confirmText),
            confirmButtonText (confirmButtonText),
            action (action),
            alwaysConsume (alwaysConsume) { this->text = text; }

        SafeMenuItem (
            std::string text,
            std::function<void ()> action,
            bool alwaysConsume = false,
            std::string confirmText = defaultConfirmText
        ) : SafeMenuItem (text, text, action, alwaysConsume, confirmText) { }

        void onAction (const rack::event::Action& e) override;
    };

    template<typename TTextField = rack::ui::TextField>
    TTextField* createEventTextField (std::string text, std::string placeholder, std::function<bool(std::string)> action, bool alwaysConsume = false) {
        struct EventTextField : TTextField {
            std::function<bool(std::string)> eventAction = nullptr;
            bool alwaysConsume;

            void step () override {
                // Keep selected.
                APP->event->setSelectedWidget (this);
                TTextField::step ();
            }

            void onSelectKey (const rack::event::SelectKey &e) override {
                if (e.action == GLFW_PRESS && (e.key == GLFW_KEY_ENTER || e.key == GLFW_KEY_KP_ENTER)) {
                    auto consumed = false;
                    if (eventAction) {
                        if (eventAction (this->text)) {
                            e.consume (this);
                            consumed = true;
                        }
                    }

                    if (alwaysConsume && !consumed)
                        e.consume (this);
                }

                if (!e.getTarget ())
                    TTextField::onSelectKey (e);
            }
        };

        auto textField = new EventTextField;
        textField->setText (text);
        textField->placeholder = placeholder;
        textField->eventAction = action;
        textField->alwaysConsume = alwaysConsume;

        return textField;
    }
}
}