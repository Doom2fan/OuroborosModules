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

#include "ColorDisplayWidget.hpp"

#include "../UI/MenuItems/ColorPicker.hpp"
#include "../UI/MenuItems/CommonItems.hpp"
#include "../UI/ThemeUtils.hpp"
#include "Chroma.hpp"

namespace OuroborosModules {
namespace Modules {
namespace Chroma {
    /*
     * ColorDisplayWidget
     */
    ColorDisplayWidget::ColorDisplayWidget (ChromaModule* module, rack::math::Rect newBox) {
        this->module = module;
        box = newBox;

        scrollContainer = new rack::ui::ScrollWidget;
        scrollContainer->box = rack::math::Rect (rack::math::Vec (), box.size);
        scrollContainer->hideScrollbars = true;
        addChild (scrollContainer);

        colorContainer = new rack::ui::SequentialLayout;
        colorContainer->box = rack::math::Rect (rack::math::Vec (), box.size);
        colorContainer->orientation = rack::ui::SequentialLayout::VERTICAL_ORIENTATION;
        colorContainer->margin = rack::math::Vec ();
        colorContainer->spacing = rack::math::Vec (0, 1);
        colorContainer->wrap = false;
        scrollContainer->container->addChild (colorContainer);

        currentSelectedIndex = -1;
    }

    void ColorDisplayWidget::step () {
        CableColorCollection colorCollection;
        uint32_t currentColor;
        if (module != nullptr) {
            colorCollection = module->colorManager->getCollection ();
            currentColor = module->colorManager->getCurrentColor ();
        } else {
            if (!pluginSettings.chroma_Collections.tryGetDefaultCollection (colorCollection))
                return;
            currentColor = 0;
        }

        if (colorCollection.count () > colorWidgets.size ()) {
            for (uint32_t i = colorCollection.count () - colorWidgets.size (); i > 0; i--) {
                auto colorWidget = new CableColorWidget (this);

                colorWidget->box.pos = rack::math::Vec ();
                colorWidget->box.size = rack::math::Vec (box.size.x, 25);
                colorContainer->addChild (colorWidget);

                colorWidgets.push_back (colorWidget);
            }
        } else if (colorCollection.count () < colorWidgets.size ()) {
            for (uint32_t i = colorWidgets.size () - colorCollection.count (); i > 0; i--) {
                colorWidgets.back ()->requestDelete ();
                colorWidgets.pop_back ();
            }
        }

        for (uint32_t i = 0; i < colorCollection.count (); i++) {
            auto colorWidget = colorWidgets [i];
            colorWidget->setColor (i, colorCollection [i]);
            colorWidget->isSelected = i == currentColor;
        }

        if (currentSelectedIndex != currentColor && currentSelectedIndex < colorCollection.count ()) {
            scrollContainer->scrollTo (colorWidgets [currentColor]->box);
            currentSelectedIndex = currentColor;
        }

        Widget::step ();

        auto sizeY = scrollContainer->box.size.y;
        for (auto widget : colorWidgets)
            sizeY = std::fmax (widget->box.pos.y + widget->box.size.y, sizeY);
        colorContainer->box.size.y = sizeY;
    }

    /*
     * CableColorWidget
     */
    CableColorWidget::CableColorWidget (ColorDisplayWidget* colorDisplay) {
        colorDisplayWidget = colorDisplay;
        theme = Theme::getCurrentTheme ().getThemeInstance ();
    }

    std::shared_ptr<CableColorManager>& CableColorWidget::getColorManager () {
        return colorDisplayWidget->module->colorManager;
    }

    void CableColorWidget::setColor (int index, const CableColor& color) {
        this->index = index;
        this->color = color;
    }

    void CableColorWidget::onButton (const rack::event::Button& e) {
        Widget::onButton (e);

        if (colorDisplayWidget == nullptr || colorDisplayWidget->module == nullptr)
            return;

        if (e.button == GLFW_MOUSE_BUTTON_LEFT && e.action == GLFW_PRESS) {
            getColorManager ()->setCurrentColor (index, true, false);
            e.consume (this);
        } else if (e.button == GLFW_MOUSE_BUTTON_RIGHT && e.action == GLFW_PRESS) {
            auto menu = rack::createMenu ();
            createContextMenu (menu);
            e.consume (this);
        }
    }

    void CableColorWidget::draw (const DrawArgs& args) {
        using rack::math::Vec;

        Widget::draw (args);

        // Draw the background.
        auto styleBG = Theme::getStyle<Theme::Styles::cable_color_display_BG> (theme);
        auto bgPaint = !isSelected
                     ? getColorPaint (rack::color::BLACK)
                     : getColorPaint (color.color);
        if (!isSelected && styleBG != nullptr && styleBG->hasFill ())
            bgPaint = styleBG->getFill ().getNVGPaint (bgPaint);

        nvgSave (args.vg);
        nvgAlpha (args.vg, .35);
        nvgBeginPath (args.vg);
        nvgRoundedRect (args.vg, 0, 0, VEC_ARGS (box.size), 1.5);
        nvgFillPaint (args.vg, bgPaint);
        nvgFill (args.vg);
        nvgRestore (args.vg);

        // Draw the circle.
        nvgSave (args.vg);
        nvgBeginPath (args.vg);
        auto circleRadius = box.size.y / 2. * .75;
        auto circlePos = Vec (4 + circleRadius, box.size.y / 2.);
        nvgCircle (args.vg, VEC_ARGS (circlePos), circleRadius);
        nvgStrokeColor (args.vg, color.color);
        nvgStrokeWidth (args.vg, 2);
        nvgStroke (args.vg);
        if (isSelected) {
            nvgFillColor (args.vg, color.color);
            nvgFill (args.vg);
        }
        nvgRestore (args.vg);

        if (!pluginSettings.chroma_DisplayKeys && color.label.empty () && !color.key.isMapped ())
            return;

        // Configure the text.
        nvgSave (args.vg);
        auto font = APP->window->loadFont (rack::asset::plugin (pluginInstance, "res/fonts/Inconsolata_Condensed-Medium.ttf"));
        nvgFontFaceId (args.vg, font->handle);
        nvgFontSize (args.vg, 13);
        nvgTextLetterSpacing (args.vg, 0);
        nvgTextLineHeight (args.vg, 1);

        auto textPos = Vec (circlePos.x + circleRadius + 4, box.size.y / 2.);

        auto labelTextStart = color.label.c_str ();
        auto labelTextEnd = labelTextStart + color.label.length ();

        auto keyText = pluginSettings.chroma_DisplayKeys && color.key.isMapped () ? color.key.keyText () : "";
        auto keyTextStart = keyText.c_str ();
        auto keyTextEnd = keyTextStart + keyText.length ();

        // Draw the text.
        auto textPaint = isSelected
                       ? getColorPaint (rack::color::WHITE)
                       : Theme::getTextPaint<Theme::Styles::cable_color_display_text> (theme);
        auto shadowPaint = getColorPaint (rack::color::BLACK);

        static constexpr float blurAmount = 4;
        static constexpr float blurSteps = 4;
        static constexpr float blurStepAmount = blurAmount / blurSteps;
        for (int i = 0; i <= blurSteps; i++) {
            nvgSave (args.vg);

            nvgFontBlur (args.vg, blurStepAmount * (blurSteps - i));
            nvgFillPaint (args.vg, (i < blurSteps) ? shadowPaint : textPaint);
            nvgAlpha (args.vg, (i < blurSteps) ? blurStepAmount : 1.);

            if (!pluginSettings.chroma_DisplayKeys) {
                // Centered label.
                nvgTextAlign (args.vg, NVG_ALIGN_MIDDLE | NVG_ALIGN_LEFT);
                nvgText (args.vg, VEC_ARGS (textPos), labelTextStart, labelTextEnd);
            } else  {
                // Label.
                if (!color.label.empty ()) {
                    nvgTextAlign (args.vg, NVG_ALIGN_TOP | NVG_ALIGN_LEFT);
                    nvgText (args.vg, textPos.x, 0, labelTextStart, labelTextEnd);
                }

                // Key mapping.
                if (color.key.isMapped ()) {
                    nvgTextAlign (args.vg, NVG_ALIGN_BOTTOM | NVG_ALIGN_RIGHT);
                    nvgText (args.vg, box.size.x, box.size.y, keyTextStart, keyTextEnd);
                }
            }

            nvgRestore (args.vg);
        }
        nvgRestore (args.vg);
    }

    void CableColorWidget::createContextMenu (rack::ui::Menu* menu) {
        // Label.
        auto labelField = UI::createEventTextField (
            color.label,
            "Label...",
            [=] (std::string text) {
                getColorManager ()->setColorLabel (index, text);
                return true;
            }
        );
        labelField->box.size.x = 250;
        labelField->setMaxLength (22);
        menu->addChild (labelField);

        // Color.
        struct ColorPickerMenu : UI::ColorPickerMenuItem<UI::ColorMenuItem> {
            std::shared_ptr<CableColorManager> colorManager;
            int32_t index;

            ColorPickerMenu (std::shared_ptr<CableColorManager> colorManager, int32_t index, NVGcolor color)
                : _WidgetBase (color) {
                text = "     Color";

                this->colorManager = colorManager;
                this->index = index;
                this->color = color;
            }

            void onApply (NVGcolor newColor) override { colorManager->setColor (index, newColor); }

            void onCancel (NVGcolor newColor) override { }
        };
        menu->addChild (new ColorPickerMenu (getColorManager (), index, color.color));

        // Key mapping.
        auto keyName = color.label;
        if (keyName == "")
            keyName = fmt::format (FMT_STRING ("cable color #{}"), index + 1);

        menu->addChild (rack::createMenuItem (
            "Set key mapping",
            color.key.keyText (),
            [=] { getColorManager ()->setLearnMode (keyName, index); },
            false, true
        ));
        menu->addChild (rack::createMenuItem (
            "Unset key mapping",
            "",
            [=] { getColorManager ()->unsetColorKey (index); },
            !color.key.isMapped ()
        ));
        menu->addChild (rack::createMenuItem (
            "Delete color",
            "",
            [=] { getColorManager ()->removeColor (index); }
        ));
    }
}
}
}