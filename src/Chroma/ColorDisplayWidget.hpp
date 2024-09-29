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
#include "CableColorManager.hpp"

#include <rack_themer.hpp>

namespace OuroborosModules::Modules::Chroma {
    struct ChromaModule;
    struct ColorDisplayWidget;

    struct CableColorWidget : rack_themer::ThemedWidgetBase<rack::widget::Widget> {
      private:
        ColorDisplayWidget* colorDisplayWidget;
        int index;
        CableColor color;

        std::shared_ptr<rack_themer::RackTheme> theme;

        std::shared_ptr<CableColorManager>& getColorManager ();

      public:
        bool isSelected;

        CableColorWidget (ColorDisplayWidget* colorDisplay);

        void setColor (int index, const CableColor& color);

        void draw (const DrawArgs& args) override;
        void onButton (const rack::event::Button& e) override;
        void onThemeChanged (std::shared_ptr<rack_themer::RackTheme> theme) override {
            _ThemedWidgetBase::onThemeChanged (theme);
            this->theme = theme;
        }

        void createContextMenu (rack::ui::Menu* menu);
    };

    struct ColorDisplayWidget : rack::widget::Widget {
        friend CableColorWidget;

      private:
        std::vector<CableColorWidget*> colorWidgets;
        rack::ui::ScrollWidget* scrollContainer;
        rack::ui::SequentialLayout* colorContainer;
        ChromaModule* module;
        uint32_t currentSelectedIndex;

        ColorDisplayWidget (const ColorDisplayWidget& x) = delete;
        void operator= (const ColorDisplayWidget& x) = delete;

      public:
        ColorDisplayWidget (ChromaModule* module, rack::math::Rect newBox);

        void step () override;
    };
}