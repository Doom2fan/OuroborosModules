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

#include "WidgetBase.hpp"

#include "../ModuleBase.hpp"
#include "MenuItems/ColorPicker.hpp"
#include "MenuItems/CommonItems.hpp"

namespace OuroborosModules::Widgets {
    std::string getLocalThemeLabel (ThemeId themeId) {
        if (themeId.isUnknown ())
            return "Use default theme";
        return themeId.getDisplayName ();
    }

    std::string getLocalEmblemLabel (EmblemId emblemId) {
        if (emblemId.isUnknown ())
            return "Use default emblem";
        return emblemId.getDisplayName ();
    }

    void HistoryThemeChange::undo () {
        auto module = dynamic_cast<ModuleBase*> (APP->engine->getModule (moduleId));
        if (module == nullptr)
            return;

        module->theme_Override = oldTheme;
    }

    void HistoryThemeChange::redo () {
        auto module = dynamic_cast<ModuleBase*> (APP->engine->getModule (moduleId));
        if (module == nullptr)
            return;

        module->theme_Override = newTheme;
    }

    void HistoryEmblemChange::undo () {
        auto module = dynamic_cast<ModuleBase*> (APP->engine->getModule (moduleId));
        if (module == nullptr)
            return;

        module->theme_Emblem = oldEmblem;
    }

    void HistoryEmblemChange::redo () {
        auto module = dynamic_cast<ModuleBase*> (APP->engine->getModule (moduleId));
        if (module == nullptr)
            return;

        module->theme_Emblem = newEmblem;
    }

    rack::ui::MenuItem* createColorList (
        std::string menuTitle,
        DisplayColor* currentColor, std::function<void (DisplayColor, DisplayColor)> setColor,
        DisplayColor* defaultColor, DisplayColor* globalColor,
        bool directDefault
    ) {
        using rack::ui::Menu;
        using rack::createSubmenuItem;
        using rack::createCheckMenuItem;

        struct DynamicColorItem : UI::ColorMenuItem {
            std::function<RGBColor ()> getColor;

            void step () override {
                ColorMenuItem::step ();
                color = getColor ();
            }
        };

        struct CustomColorPickerMenu : UI::ColorPickerMenuItem<UI::ColorMenuItem> {
            std::function<RGBColor ()> getColor;
            std::function<void (RGBColor)> setColor;

            CustomColorPickerMenu (std::function<RGBColor ()> getColor, std::function<void (RGBColor)> setColor)
                : _WidgetBase (getColor ()), getColor (getColor), setColor (setColor) {
                text = "     Custom";
                color = getColor ();
            }

            void onApply (NVGcolor newColor) override { setColor (newColor); }

            void onCancel (NVGcolor newColor) override { }
        };

        auto pickerMenu = createSubmenuItem<DynamicColorItem> (
            fmt::format (FMT_STRING ("     {}"), menuTitle), "",
            [=] (Menu* menu) {
                if (defaultColor != nullptr) {
                    auto defaultColorItem = createCheckMenuItem<DynamicColorItem> (
                        "     Default", "",
                        [=] { return (!directDefault) ? currentColor->checkDefault () : (*currentColor == *defaultColor); },
                        [=] { setColor (*currentColor, !directDefault ? DisplayColor::createDefault () : *defaultColor); }
                    );
                    defaultColorItem->getColor = [=] { return defaultColor->getColor (nullptr, globalColor); };
                    menu->addChild (defaultColorItem);
                }

                if (globalColor != nullptr) {
                    auto globalColorItem = createCheckMenuItem<DynamicColorItem> (
                        "     Global default", "",
                        [=] { return currentColor->checkGlobal (); },
                        [=] { setColor (*currentColor, DisplayColor::createGlobal ()); }
                    );
                    globalColorItem->getColor = [=] { return globalColor->getColor (nullptr, nullptr); };
                    menu->addChild (globalColorItem);
                }

                menu->addChild (new CustomColorPickerMenu (
                    [=] { return currentColor->getColor (defaultColor, globalColor); },
                    [=] (RGBColor color) { setColor (*currentColor, DisplayColor::createLocal (color)); }
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
                        [=] { return currentColor->checkLocal () && color == currentColor->getLocal (); },
                        [=] { setColor (*currentColor, DisplayColor::createLocal (color)); }
                    );
                    menuItem->color = color;
                    menu->addChild (menuItem);
                }
            }
        );
        pickerMenu->getColor = [=] { return currentColor->getColor (defaultColor, globalColor); };

        return pickerMenu;
    }
}