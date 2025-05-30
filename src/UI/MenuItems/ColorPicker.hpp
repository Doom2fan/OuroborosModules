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

#include "../../PluginDef.hpp"
#include "CommonItems.hpp"
#include "TextField.hpp"

namespace OuroborosModules::UI {
    template<typename TBaseItem = rack::ui::MenuItem>
    struct ColorPickerMenuItem : TBaseItem {
      public:
        typedef ColorPickerMenuItem<TBaseItem> _WidgetBase;

      private:
        NVGcolor currentColor;
        TextField* hexColorField = nullptr;

        struct ColorDisplay : rack::ui::MenuItem {
          private:
            NVGcolor* colorPointer;

          public:
            ColorDisplay (NVGcolor* colorPointer) : colorPointer (colorPointer) {  }

            void onDragDrop (const rack::event::DragDrop& e) override { return; }

            void draw (const DrawArgs& args) override {
                MenuItem::draw (args);

                // Color circle
                nvgBeginPath (args.vg);
                auto boxMargin = rack::math::Vec (3, 3);
                auto boxSize = box.size - boxMargin.mult (2);
                nvgRoundedRect (args.vg, VEC_ARGS (boxMargin), VEC_ARGS (boxSize), 2);

                nvgFillColor (args.vg, *colorPointer);
                nvgFill (args.vg);

                nvgStrokeWidth (args.vg, 1.);
                nvgStrokeColor (args.vg, rack::color::mult (*colorPointer, .5));
                nvgStroke (args.vg);
            }
        };

        struct ColorSlider : rack::ui::Slider {
            struct ColorQuantity : rack::Quantity {
              private:
                std::string label;
                float *colorPointer;
                float defaultValue;
                std::function<void ()> onChange;

              public:
                ColorQuantity (std::string label, float* colorPointer, std::function<void ()> onChange) {
                    this->label = label;
                    this->colorPointer = colorPointer;
                    this->defaultValue = *colorPointer;
                    this->onChange = onChange;
                }

                void setValue (float value) override {
                    *colorPointer = std::clamp (value, getMinValue (), getMaxValue ());
                    if (onChange != nullptr)
                        onChange ();
                }
                float getValue () override { return *colorPointer; }
                float getMinValue () override { return 0; }
                float getMaxValue () override { return 1; }

                std::string getLabel () override { return label; }
                float getDisplayValue () override { return getValue () * 100; }
                void setDisplayValue (float displayValue) override { setValue (displayValue / 100); }
                float getDefaultValue () override { return defaultValue; }
                int getDisplayPrecision () override { return 3; }
                std::string getUnit () override { return "%"; }
            };

            ColorSlider (std::string label, float* colorPointer, std::function<void ()> onChange) {
                quantity = new ColorQuantity (label, colorPointer, onChange);
                box.size.x = 200;
            }

            ~ColorSlider () {
                delete quantity;
            }
        };

      public:
        bool cancelOnClose = false;

      protected:
        void closeMenu () {
            if (auto overlay = this->template getAncestorOfType<rack::ui::MenuOverlay> ())
                overlay->requestDelete ();
        }

        ColorPickerMenuItem (NVGcolor defaultColor) {
            this->currentColor = defaultColor;
        }

        void onRemove (const rack::event::Remove& e) override {
            if (cancelOnClose)
                onCancel (currentColor);

            TBaseItem::onRemove (e);
        }

        virtual void onApply (NVGcolor newColor) = 0;
        virtual void onCancel (NVGcolor newColor) = 0;

        virtual void onColorChange (NVGcolor newColor) { }

      private:
        void callApply () {
            onApply (currentColor);
            cancelOnClose = false;
        }

        void callCancel () {
            onCancel (currentColor);
        }

        void updateHex () {
            if (hexColorField == nullptr)
                return;

            hexColorField->setText (rack::color::toHexString (currentColor));
        }

        void callOnChange () {
            updateHex ();
            onColorChange (currentColor);
        }

        void setHexColor (std::string hexColor) {
            if (hexColor.length () != 7)
                return;

            if (hexColor [0] != '#')
                return;

            for (int i = 1; i < 7; i++) {
                auto ch = hexColor [i];
                if (!(ch >= 'a' && ch <= 'f') &&
                    !(ch >= 'A' && ch <= 'F') &&
                    !(ch >= '0' && ch <= '9'))
                    return;
            }

            currentColor = rack::color::fromHexString (hexColor);
            callOnChange ();
        }

        rack::ui::Menu* createChildMenu () override {
            auto menu = new rack::ui::Menu;

            menu->addChild (new ColorDisplay (&currentColor));

            menu->addChild (new rack::ui::MenuSeparator);
            hexColorField = createEventTextField (
                "", "Hex color...",
                [=] (std::string value) { setHexColor (value); return false; },
                false, false
            );
            hexColorField->box.size.x = 200;
            menu->addChild (hexColorField);
            auto changeFunction = [=] { callOnChange (); };
            menu->addChild (new ColorSlider ("Red", &currentColor.r, changeFunction));
            menu->addChild (new ColorSlider ("Green", &currentColor.g, changeFunction));
            menu->addChild (new ColorSlider ("Blue", &currentColor.b, changeFunction));

            menu->addChild (new rack::ui::MenuSeparator);
            menu->addChild (rack::createMenuItem ("Accept", "", [=] { callApply (); }));
            menu->addChild (rack::createMenuItem ("Cancel", "", [=] { callCancel (); }));

            updateHex ();

            return menu;
        }
    };
}