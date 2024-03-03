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
#include "../Utils.hpp"
#include "ThemeUtils.hpp"
#include <rack_themer.hpp>

std::string getLocalThemeLabel (ThemeKind theme);
std::string getLocalEmblemLabel (EmblemKind emblem);

template<typename T>
rack::ui::MenuItem* createThemeMenuItem (std::string text, std::string rightText, T* enumPtr, T selectedVal) {
    return rack::createCheckMenuItem (text, rightText, [=] { return *enumPtr == selectedVal; }, [=] { *enumPtr = selectedVal; });
}

template<typename TSelf, typename TModule, typename TBase = rack::app::ModuleWidget>
struct ModuleWidgetBase : TBase, rack_themer::IThemedWidget, rack_themer::SvgHelper<ModuleWidgetBase<TSelf, TModule, TBase>> {
  protected:
    TModule* module;

    std::string panelName;
    ThemeKind curTheme = ThemeKind::Unknown;
    EmblemKind curEmblem = EmblemKind::Unknown;

  public:
    ThemeKind getLocalTheme () {
        if (module != nullptr && module->theme_Override != ThemeKind::Unknown)
            return module->theme_Override;
        return getCurrentTheme ();
    }
    EmblemKind getLocalEmblem () {
        if (module != nullptr && module->theme_Emblem != EmblemKind::Unknown)
            return module->theme_Emblem;
        return pluginSettings.global_DefaultEmblem;
    }

    ModuleWidgetBase (TModule* module, std::string panelName) {
        this->module = module;
        this->panelName = panelName;

        this->setModule (module);

        updateTheme ();
        updateEmblem ();
    }

    void updateTheme () {
        auto theme = getLocalTheme ();
        if (curTheme == theme)
            return;

        curTheme = theme;
        onChangeTheme (theme);
    }

    void updateEmblem () {
        auto emblem = getLocalEmblem ();
        if (curEmblem == emblem)
            return;

        curEmblem = emblem;
        onChangeEmblem (emblem);
    }

    virtual void onChangeTheme (ThemeKind theme) { handleThemeChange (this, getTheme (theme), true); }

    virtual void onChangeEmblem (EmblemKind emblem) { }

    void onThemeChanged (std::shared_ptr<rack_themer::RackTheme> theme) override {
        this->loadPanel (getThemedSvg (panelName, theme));
    }

    virtual void createPluginSettingsMenu (TSelf* widget, rack::ui::Menu* menu) {
        menu->addChild (rack::createSubmenuItem ("Theme settings", "", [=] (rack::ui::Menu* menu) {
            menu->addChild (rack::createMenuLabel ("Default light theme"));
            for (auto i = ThemeKind::FirstTheme; i < ThemeKind::ThemeCount; i = static_cast<ThemeKind> (static_cast<int> (i) + 1))
                menu->addChild (createThemeMenuItem (getThemeLabel (i), "", &pluginSettings.global_ThemeLight, i));

            menu->addChild (rack::createMenuLabel ("Default dark theme"));
            for (auto i = ThemeKind::FirstTheme; i < ThemeKind::ThemeCount; i = static_cast<ThemeKind> (static_cast<int> (i) + 1))
                menu->addChild (createThemeMenuItem (getThemeLabel (i), "", &pluginSettings.global_ThemeDark, i));

            menu->addChild (rack::createMenuLabel ("Default emblem"));
            for (auto i = EmblemKind::FirstEmblem; i < EmblemKind::EmblemCount; i = static_cast<EmblemKind> (static_cast<int> (i) + 1))
                menu->addChild (createThemeMenuItem (getEmblemLabel (i), "", &pluginSettings.global_DefaultEmblem, i));
        }));
    }

    void step () override {
        updateTheme ();
        updateEmblem ();

        TBase::step ();
    }

    void appendContextMenu (rack::ui::Menu* menu) override {
        using rack::Menu;
        using rack::createMenuLabel;
        using rack::createSubmenuItem;
        using rack::createCheckMenuItem;

        auto createThemeOverrideItem = [=] (std::string name, ThemeKind theme) {
            return createCheckMenuItem (name, "", [=] { return module->theme_Override == theme; }, [=] { module->theme_Override = theme; updateTheme (); });
        };
        auto createEmblemOverrideItem = [=] (std::string name, EmblemKind emblem) {
            return createCheckMenuItem (name, "", [=] { return module->theme_Emblem == emblem; }, [=] { module->theme_Emblem = emblem; updateEmblem (); });
        };

        TBase::appendContextMenu (menu);
        menu->addChild (createSubmenuItem ("Global settings", "", [=] (Menu* menu) { createPluginSettingsMenu (dynamic_cast<TSelf*> (this), menu); }));

        menu->addChild (createSubmenuItem ("Local style", "", [=] (Menu* menu) {
            menu->addChild (createMenuLabel ("Theme"));
            menu->addChild (createThemeOverrideItem ("Default", ThemeKind::Unknown));
            for (auto i = ThemeKind::FirstTheme; i < ThemeKind::ThemeCount; i = static_cast<ThemeKind> (static_cast<int> (i) + 1))
                menu->addChild (createThemeOverrideItem (getThemeLabel (i), i));

            menu->addChild (createMenuLabel ("Emblem"));
            menu->addChild (createEmblemOverrideItem ("Default", EmblemKind::Unknown));
            for (auto i = EmblemKind::FirstEmblem; i < EmblemKind::EmblemCount; i = static_cast<EmblemKind> (static_cast<int> (i) + 1))
                menu->addChild (createEmblemOverrideItem (getEmblemLabel (i), i));
        }));

        /*theme_Override
theme_Emblem*/
    }
};

#if false
template<typename TModule, typename TBase>
struct ChildWidgetBase : TBase {
  protected:
    TModule* module;
    ThemeKind curTheme = ThemeKind::Unknown;

  public:
    virtual void onChangeTheme (ThemeKind newTheme) { }
};
#endif