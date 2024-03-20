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

namespace OuroborosModules {
namespace Widgets {
    std::string getLocalThemeLabel (ThemeKind theme);
    std::string getLocalEmblemLabel (EmblemKind emblem);

    template<typename T>
    rack::ui::MenuItem* createThemeMenuItem (std::string text, std::string rightText, T* enumPtr, T selectedVal) {
        return rack::createCheckMenuItem (text, rightText, [=] { return *enumPtr == selectedVal; }, [=] { *enumPtr = selectedVal; });
    }

    struct HistoryThemeChange : rack::history::ModuleAction {
        ThemeKind oldTheme;
        ThemeKind newTheme;

        HistoryThemeChange () { name = "change theme override"; }

        void undo () override;
        void redo () override;
    };

    struct HistoryEmblemChange : rack::history::ModuleAction {
        EmblemKind oldEmblem;
        EmblemKind newEmblem;

        HistoryEmblemChange () { name = "change emblem override"; }

        void undo () override;
        void redo () override;
    };

    template<typename TSelf, typename TModule, typename TBase = rack::app::ModuleWidget>
    struct ModuleWidgetBase : TBase, rack_themer::IThemedWidget, rack_themer::SvgHelper<ModuleWidgetBase<TSelf, TModule, TBase>> {
      public:
        typedef ModuleWidgetBase<TSelf, TModule, TBase> _WidgetBase;
        typedef TModule _ModuleType;

      protected:
        TModule* module;

        std::string panelName;
        ThemeKind curTheme = ThemeKind::Unknown;
        EmblemKind curEmblem = EmblemKind::Unknown;

      public:
        ThemeKind getLocalTheme () {
            if (module != nullptr && module->theme_Override != ThemeKind::Unknown)
                return module->theme_Override;
            return Theme::getCurrentTheme ();
        }
        EmblemKind getLocalEmblem () {
            if (module != nullptr && module->theme_Emblem != EmblemKind::Unknown)
                return module->theme_Emblem;
            return pluginSettings.global_DefaultEmblem;
        }

      protected:
        void constructor (TModule* module, std::string panelName) {
            this->module = module;
            this->panelName = panelName;

            this->loadPanel (Theme::getThemedSvg (panelName, nullptr));
            this->setModule (module);

            curTheme = ThemeKind::INVALID;
            curEmblem = EmblemKind::INVALID;

            initializeWidget ();

            updateTheme ();
            updateEmblem ();
        }

        virtual void initializeWidget () = 0;

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

        void setTheme (ThemeKind theme) {
            auto oldTheme = module->theme_Override;
            module->theme_Override = theme;

            auto themeChange = new HistoryThemeChange;

            themeChange->moduleId = module->id;
            themeChange->oldTheme = oldTheme;
            themeChange->newTheme = theme;

            APP->history->push (themeChange);

            updateTheme ();
        }

        void setEmblem (EmblemKind emblem) {
            auto oldEmblem = module->theme_Emblem;
            module->theme_Emblem = emblem;

            auto emblemChange = new HistoryEmblemChange;

            emblemChange->moduleId = module->id;
            emblemChange->oldEmblem = oldEmblem;
            emblemChange->newEmblem = emblem;

            APP->history->push (emblemChange);

            updateEmblem ();
        }

        virtual void onChangeTheme (ThemeKind theme) { handleThemeChange (this, Theme::getTheme (theme), true); }

        virtual void onChangeEmblem (EmblemKind emblem) { }

        void onThemeChanged (std::shared_ptr<rack_themer::RackTheme> theme) override {
            this->loadPanel (Theme::getThemedSvg (panelName, theme));
        }

        virtual void createPluginSettingsMenu (rack::ui::Menu* menu) {
            menu->addChild (rack::createSubmenuItem ("Theme settings", "", [] (rack::ui::Menu* menu) {
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

        virtual void createLocalStyleMenu (rack::ui::Menu* menu) {
            using rack::ui::Menu;
            using rack::createMenuLabel;
            using rack::createSubmenuItem;
            using rack::createCheckMenuItem;

            auto createThemeOverrideItem = [=] (std::string name, ThemeKind theme) {
                return createCheckMenuItem (name, "", [=] { return module->theme_Override == theme; }, [=] { setTheme (theme); });
            };
            auto createEmblemOverrideItem = [=] (std::string name, EmblemKind emblem) {
                return createCheckMenuItem (name, "", [=] { return module->theme_Emblem == emblem; }, [=] { setEmblem (emblem); });
            };

            menu->addChild (createMenuLabel ("Theme"));
            menu->addChild (createThemeOverrideItem ("Default", ThemeKind::Unknown));
            for (auto i = ThemeKind::FirstTheme; i < ThemeKind::ThemeCount; i = static_cast<ThemeKind> (static_cast<int> (i) + 1))
                menu->addChild (createThemeOverrideItem (getThemeLabel (i), i));

            menu->addChild (createMenuLabel ("Emblem"));
            menu->addChild (createEmblemOverrideItem ("Default", EmblemKind::Unknown));
            for (auto i = EmblemKind::FirstEmblem; i < EmblemKind::EmblemCount; i = static_cast<EmblemKind> (static_cast<int> (i) + 1))
                menu->addChild (createEmblemOverrideItem (getEmblemLabel (i), i));
        }

        void step () override {
            updateTheme ();
            updateEmblem ();

            TBase::step ();
        }

        void appendContextMenu (rack::ui::Menu* menu) override {
            using rack::ui::Menu;
            using rack::createMenuLabel;
            using rack::createSubmenuItem;
            using rack::createCheckMenuItem;

            TBase::appendContextMenu (menu);
            menu->addChild (createSubmenuItem ("Global settings", "", [&] (Menu* menu) { createPluginSettingsMenu (menu); }));
            menu->addChild (createSubmenuItem ("Local style", "", [&] (Menu* menu) { createLocalStyleMenu (menu); }));
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
}
}