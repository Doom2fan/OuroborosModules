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
#include <SvgHelper.hpp>

std::string getThemedSvg (std::string filePath, ThemeKind theme);
std::string getEmblem (EmblemKind emblem);

std::string getThemeLabel (ThemeKind theme);
std::string getLocalThemeLabel (ThemeKind theme);

std::string getEmblemLabel (EmblemKind emblem);
std::string getLocalEmblemLabel (EmblemKind emblem);

template<typename T>
MenuItem* createThemeMenuItem (std::string text, std::string rightText, T* enumPtr, T selectedVal) {
    return createCheckMenuItem (text, rightText, [=] { return *enumPtr == selectedVal; }, [=] { *enumPtr = selectedVal; });
}

template<typename TSelf, typename TModule, typename TBase = ModuleWidget>
struct ModuleWidgetBase : TBase, SvgHelper<ModuleWidgetBase<TSelf, TModule, TBase>> {
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

    std::string getLocalThemedSvg (std::string filePath) { return getThemedSvg (filePath, curTheme); }

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

    virtual void onChangeTheme (ThemeKind theme) {
        this->loadPanel (asset::plugin (pluginInstance, getLocalThemedSvg (panelName)));
    }

    virtual void onChangeEmblem (EmblemKind emblem) { }

    virtual void createPluginSettingsMenu (TSelf* widget, Menu* menu) {
        menu->addChild (createSubmenuItem ("Theme settings", "", [=] (Menu* menu) {
            menu->addChild (createMenuLabel ("Default light theme"));
            menu->addChild (createThemeMenuItem ("Light", "", &pluginSettings.global_ThemeLight, ThemeKind::Light));
            menu->addChild (createThemeMenuItem ("Dark", "", &pluginSettings.global_ThemeLight, ThemeKind::Dark));

            menu->addChild (createMenuLabel ("Default dark theme"));
            menu->addChild (createThemeMenuItem ("Light", "", &pluginSettings.global_ThemeDark, ThemeKind::Light));
            menu->addChild (createThemeMenuItem ("Dark", "", &pluginSettings.global_ThemeDark, ThemeKind::Dark));

            menu->addChild (createMenuLabel ("Default emblem"));
            menu->addChild (createThemeMenuItem ("None", "", &pluginSettings.global_DefaultEmblem, EmblemKind::None));
            menu->addChild (createThemeMenuItem ("Dragon", "", &pluginSettings.global_DefaultEmblem, EmblemKind::Dragon));
            menu->addChild (createThemeMenuItem ("Bleeding eye", "", &pluginSettings.global_DefaultEmblem, EmblemKind::BleedingEye));
        }));
    }

    void step () override {
        updateTheme ();
        updateEmblem ();

        TBase::step ();
    }

    void appendContextMenu (Menu* menu) override {
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
            menu->addChild (createThemeOverrideItem ("Light", ThemeKind::Light));
            menu->addChild (createThemeOverrideItem ("Dark", ThemeKind::Dark));

            menu->addChild (createMenuLabel ("Emblem"));
            menu->addChild (createEmblemOverrideItem ("Default", EmblemKind::Unknown));
            menu->addChild (createEmblemOverrideItem ("None", EmblemKind::None));
            menu->addChild (createEmblemOverrideItem ("Dragon", EmblemKind::Dragon));
            menu->addChild (createEmblemOverrideItem ("Bleeding eye", EmblemKind::BleedingEye));
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