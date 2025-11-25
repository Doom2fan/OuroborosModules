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

#include "STVCA.hpp"

#include "../UI/MenuItems/ColorPicker.hpp"
#include "../UI/MenuItems/CommonItems.hpp"

#include <fmt/format.h>

namespace OuroborosModules::Modules::STVCA {
    STVCAWidget::STVCAWidget (STVCAModule* module) { constructor (module, "panels/ST-VCA"); }

    struct STVCASlider : rack::app::SliderKnob {
        void drawLayer (const DrawArgs& args, int layer) override {
            if (layer != 1)
                return;

            auto module = dynamic_cast<STVCAModule*> (this->module);

            auto r = box.zeroPos ();

            auto channels = module ? module->lastChannels : 1;
            auto pq = getParamQuantity ();
            auto value = pq ? pq->getValue () : 1.f;

            // Segment value.
            if (value >= .005f) {
                nvgBeginPath (args.vg);
                nvgRect (args.vg,
                    r.pos.x,
                    r.pos.y + r.size.y * (1 - value),
                    r.size.x,
                    r.size.y * value
                );
                nvgFillColor (args.vg, rack::color::mult (rack::color::WHITE, .25f));
                nvgFill (args.vg);
            }

            // Segment gain
            nvgBeginPath (args.vg);
            auto segmentFill = false;
            for (auto c = 0; c < channels; c++) {
                auto gain = module ? module->lastGains [c] : 1.f;

                if (gain >= .005f) {
                    segmentFill = true;
                    nvgRect (args.vg,
                        r.pos.x + r.size.x * c / channels,
                        r.pos.y + r.size.y * (1 - gain),
                        r.size.x / channels,
                        r.size.y * gain
                    );
                }
            }

            auto displayColor = (module != nullptr && !module->displayColorUseDefault)
                              ? module->displayColor
                              : pluginSettings.stVCA_DefaultDisplayColor;
            nvgFillColor (args.vg, (NVGcolor) displayColor);
            // If nvgFill is called with 0 path elements, it can fill other undefined paths.
            if (segmentFill)
                nvgFill (args.vg);

            // Invisible separators.
            const int segs = 25;
            nvgBeginPath (args.vg);
            for (auto i = 1; i < segs; i++) {
                nvgRect (args.vg,
                    r.pos.x - 1.f,
                    r.pos.y + r.size.y * i / segs,
                    r.size.x + 2.f,
                    1.f
                );
            }
            nvgFillColor (args.vg, nvgRGB (0x12, 0x12, 0x12));
            nvgFill (args.vg);
        }
    };

    void STVCAWidget::initializeWidget () {
        using rack::createInputCentered;
        using rack::createOutputCentered;
        using rack::createWidget;
        using rack::createWidgetCentered;
        using rack::math::Vec;
        using Widgets::CableJackInput;
        using Widgets::CableJackOutput;
        using Widgets::EmblemWidget;
        using Widgets::ScrewWidget;

        addChild (createWidget<ScrewWidget> (Vec ()));
        addChild (createWidget<ScrewWidget> (Vec (box.size.x, RACK_GRID_HEIGHT).minus (Vec (RACK_GRID_WIDTH))));

        emblemWidget = new Widgets::EmblemWidget (curEmblem, findNamed ("widgetLogo", Vec ()));
        addChild (emblemWidget);

        forEachMatched ("input_(\\d+)", [&] (std::vector<std::string> captures, Vec pos) {
            int i = stoi (captures [0]) - 1;
            addInput (createInputCentered<CableJackInput> (pos, moduleT, STVCAModule::INPUT_LEFT + i));
        });
        forEachMatched ("output_(\\d+)", [&] (std::vector<std::string> captures, Vec pos) {
            int i = stoi (captures [0]) - 1;
            addOutput (createOutputCentered<CableJackOutput> (pos, moduleT, STVCAModule::OUTPUT_LEFT + i));
        });

        auto displayBox = findNamedBox ("display", rack::math::Rect ());
        auto display = rack::createWidget<rack::LedDisplay> (Vec ());
        display->box = displayBox;
        addChild (display);

        const auto knobMarginX = 6.653f;
        const auto knobMarginY = 8.539f;
        auto slider = rack::createParam<STVCASlider> (Vec (knobMarginX, knobMarginY), moduleT, STVCAModule::PARAM_LEVEL);
        slider->box.size = displayBox.size - Vec (knobMarginX, knobMarginY).mult (2);
        display->addChild (slider);

        addChild (rack::createParamCentered<Widgets::SlideSwitch2Inverse> (findNamed ("param_Exp", Vec ()), moduleT, STVCAModule::PARAM_EXP));
    }

    void STVCAWidget::onChangeEmblem (EmblemId emblemId) {
        _WidgetBase::onChangeEmblem (emblemId);
        emblemWidget->setEmblem (emblemId);
    }

    struct HistoryChangeDisplayColor : rack::history::ModuleAction {
      private:
        struct ColorValue {
            bool isDefault;
            RGBColor color;

            ColorValue (bool isDefault, RGBColor color) : isDefault (isDefault), color (color) { }
        };

        static ColorValue createDefault () { return ColorValue (true, RGBColor ()); }
        static ColorValue createColor (RGBColor color) { return ColorValue (false, color); }
        static ColorValue createFromModule (STVCAModule* module) {
            return module->displayColorUseDefault ? createDefault () : createColor (module->displayColor);
        }

        ColorValue oldValue;
        ColorValue newValue;

        HistoryChangeDisplayColor (STVCAModule* module, ColorValue oldValue, ColorValue newValue)
            : oldValue (oldValue), newValue (newValue) {
            moduleId = module->id;
            this->name = "Set ST-VCA display color";

            setValue (newValue);
        }

      public:
        static void createDefault (STVCAModule* module) {
            APP->history->push (new HistoryChangeDisplayColor (module, createFromModule (module), createDefault ()));
        }

        static void createColor (STVCAModule* module, RGBColor color) {
            APP->history->push (new HistoryChangeDisplayColor (module, createFromModule (module), createColor (color)));
        }

        void setValue (const ColorValue& value) {
            auto module = dynamic_cast<STVCAModule*> (APP->engine->getModule (moduleId));
            if (module == nullptr)
                return;

            module->displayColorUseDefault = value.isDefault;
            module->displayColor = value.color;
        }

        void undo () override { setValue (oldValue); }
        void redo () override { setValue (newValue); }
    };

    void STVCAWidget::createLocalStyleMenu (rack::ui::Menu* menu) {
        using rack::ui::Menu;
        using rack::createSubmenuItem;
        using rack::createCheckMenuItem;

        _WidgetBase::createLocalStyleMenu (menu);

        if (moduleT == nullptr)
            return;

        menu->addChild (new rack::ui::MenuSeparator);
        struct DisplayColorPickerMenu : UI::ColorPickerMenuItem<UI::ColorMenuItem> {
            STVCAModule* module;

            DisplayColorPickerMenu (STVCAModule* module, NVGcolor color)
                : _WidgetBase (color), module (module) {
                text = "Custom";
            }

            void onApply (NVGcolor newColor) override {
                module->displayColor = newColor;
                module->displayColorUseDefault = false;
            }

            void onCancel (NVGcolor newColor) override { }
        };

        auto displayColorItem = createSubmenuItem<UI::ColorMenuItem> (
            "     Display color", "",
            [=] (Menu* menu) {


                auto defaultColorItem = createCheckMenuItem<UI::ColorMenuItem> (
                    "     Default", "",
                    [=] { return moduleT->displayColorUseDefault; },
                    [=] { HistoryChangeDisplayColor::createDefault (moduleT); }
                );
                defaultColorItem->color = pluginSettings.stVCA_DefaultDisplayColor;
                menu->addChild (defaultColorItem);
                menu->addChild (new DisplayColorPickerMenu (moduleT,
                    !moduleT->displayColorUseDefault ? moduleT->displayColor : pluginSettings.stVCA_DefaultDisplayColor
                ));

                auto firstColor = true;
                for (auto colorKVP : Colors::DisplayColors) {
                    auto name = colorKVP.first;
                    auto color = colorKVP.second;

                    if (firstColor) {
                        firstColor = false;
                        menu->addChild (new rack::ui::MenuSeparator);
                    }

                    auto menuItem = createCheckMenuItem<UI::ColorMenuItem> (
                        fmt::format (FMT_STRING ("     {}"), name), "",
                        [=] { return color == moduleT->displayColor && !moduleT->displayColorUseDefault; },
                        [=] { HistoryChangeDisplayColor::createColor (moduleT, color); }
                    );
                    menuItem->color = color;
                    menu->addChild (menuItem);
                }
            }
        );
        displayColorItem->color = !moduleT->displayColorUseDefault
                                ? moduleT->displayColor
                                : pluginSettings.stVCA_DefaultDisplayColor;
        menu->addChild (displayColorItem);
    }

    void STVCAWidget::createPluginSettingsMenu (rack::ui::Menu* menu) {
        using rack::ui::Menu;
        using rack::createSubmenuItem;
        using rack::createCheckMenuItem;

        _WidgetBase::createPluginSettingsMenu (menu);

        if (moduleT == nullptr)
            return;

        menu->addChild (new rack::ui::MenuSeparator);
        menu->addChild (rack::createMenuLabel ("Visual"));
        struct DisplayColorPickerMenu : UI::ColorPickerMenuItem<UI::ColorMenuItem> {
            STVCAModule* module;

            DisplayColorPickerMenu (STVCAModule* module, NVGcolor color)
                : _WidgetBase (color), module (module) {
                text = "Custom";
            }

            void onApply (NVGcolor newColor) override { pluginSettings.stVCA_DefaultDisplayColor = newColor; }

            void onCancel (NVGcolor newColor) override { }
        };

        auto displayColorItem = createSubmenuItem<UI::ColorMenuItem> (
            "     Default display color", "",
            [=] (Menu* menu) {
                rack::ui::MenuItem* firstColor = nullptr;
                for (auto colorKVP : Colors::DisplayColors) {
                    auto name = colorKVP.first;
                    auto color = colorKVP.second;
                    auto menuItem = createCheckMenuItem<UI::ColorMenuItem> (
                        fmt::format (FMT_STRING ("     {}"), name), "",
                        [=] { return color == pluginSettings.stVCA_DefaultDisplayColor; },
                        [=] { pluginSettings.stVCA_DefaultDisplayColor = color; }
                    );
                    menuItem->color = color;
                    menu->addChild (menuItem);
                    if (firstColor == nullptr)
                        firstColor = menuItem;
                }
                if (firstColor != nullptr) {
                    menu->addChildBelow (new DisplayColorPickerMenu (moduleT, pluginSettings.stVCA_DefaultDisplayColor), firstColor);
                    menu->addChildBelow (new rack::ui::MenuSeparator, firstColor);
                } else
                    menu->addChild (new DisplayColorPickerMenu (moduleT, pluginSettings.stVCA_DefaultDisplayColor));
            }
        );
        displayColorItem->color = pluginSettings.stVCA_DefaultDisplayColor;
        menu->addChild (displayColorItem);
    }
}