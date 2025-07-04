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

#include "../PluginDef.hpp"
#include "../Utils.hpp"
#include "ThemeUtils.hpp"

#include <rack_themer.hpp>

namespace OuroborosModules::Widgets {
    std::string getLocalThemeLabel (ThemeId themeId);
    std::string getLocalEmblemLabel (EmblemId emblemId);

    template<typename T>
    rack::ui::MenuItem* createThemeMenuItem (std::string text, std::string rightText, T* enumPtr, T selectedVal) {
        return rack::createCheckMenuItem (text, rightText, [=] { return *enumPtr == selectedVal; }, [=] { *enumPtr = selectedVal; });
    }

    struct HistoryThemeChange : rack::history::ModuleAction {
        ThemeId oldTheme;
        ThemeId newTheme;

        HistoryThemeChange (rack::engine::Module* module, ThemeId oldTheme, ThemeId newTheme)
            : oldTheme (oldTheme), newTheme (newTheme) {
            moduleId = module->id;
            name = "change theme override";
        }

        void undo () override;
        void redo () override;
    };

    struct HistoryEmblemChange : rack::history::ModuleAction {
        EmblemId oldEmblem;
        EmblemId newEmblem;

        HistoryEmblemChange (rack::engine::Module* module, EmblemId oldEmblem, EmblemId newEmblem)
            : oldEmblem (oldEmblem), newEmblem (newEmblem) {
            moduleId = module->id;
            name = "change emblem override";
        }

        void undo () override;
        void redo () override;
    };

    template<typename TModule, typename TBase = rack::app::ModuleWidget>
    struct ModuleWidgetBase : rack_themer::SvgHelper<rack_themer::ThemeHolderWidgetBase<TBase>>, rack_themer::IThemedWidget {
      public:
        typedef ModuleWidgetBase<TModule, TBase> _WidgetBase;
        typedef TModule _ModuleType;

      protected:
        TModule* moduleT = nullptr;

        std::string panelName;
        ThemeId curTheme = ThemeId::getUnknown ();
        EmblemId curEmblem = EmblemId::getUnknown ();

        ModuleWidgetBase () { }
        ModuleWidgetBase (const ModuleWidgetBase& x) = delete;
        void operator= (const ModuleWidgetBase& x) = delete;

      public:
        ThemeId getLocalTheme () {
            if (moduleT != nullptr && !moduleT->theme_Override.isUnknown ())
                return moduleT->theme_Override;
            return Theme::getCurrentTheme ();
        }
        EmblemId getLocalEmblem () {
            if (moduleT != nullptr && !moduleT->theme_Emblem.isUnknown ())
                return moduleT->theme_Emblem;
            return pluginSettings.global_DefaultEmblem;
        }

        std::shared_ptr<rack_themer::RackTheme> getTheme () override { return getLocalTheme ().getThemeInstance (); }

      protected:
        void constructor (TModule* module, std::string panelName) {
            this->moduleT = module;
            this->panelName = panelName;

            this->loadPanel (Theme::getThemedSvg (panelName, nullptr));
            this->setModule (module);

            curTheme = ThemeId::getInvalid ();
            curEmblem = EmblemId::getInvalid ();

            initializeWidget ();

            callUpdateTheme ();
            callUpdateEmblem ();
        }

        virtual void initializeWidget () = 0;

        void callUpdateTheme () {
            auto theme = getLocalTheme ();
            if (curTheme == theme)
                return;

            curTheme = theme;
            onChangeTheme (theme);
        }

        void callUpdateEmblem () {
            auto emblem = getLocalEmblem ();
            if (curEmblem == emblem)
                return;

            curEmblem = emblem;
            onChangeEmblem (emblem);
        }

        void setTheme (ThemeId themeId) {
            auto oldTheme = moduleT->theme_Override;
            moduleT->theme_Override = themeId;

            APP->history->push (new HistoryThemeChange (moduleT, oldTheme, themeId));

            callUpdateTheme ();
        }

        void setEmblem (EmblemId emblemId) {
            auto oldEmblem = moduleT->theme_Emblem;
            moduleT->theme_Emblem = emblemId;

            APP->history->push (new HistoryEmblemChange (moduleT, oldEmblem, emblemId));

            callUpdateEmblem ();
        }

        virtual void onChangeTheme (ThemeId themeId) { handleThemeChange (this, themeId.getThemeInstance (), true); }

        virtual void onChangeEmblem (EmblemId emblemId) { }

        void onThemeChanged (std::shared_ptr<rack_themer::RackTheme> theme) override {
            this->loadPanel (Theme::getThemedSvg (panelName, theme));
        }

        virtual void createPluginSettingsMenu (rack::ui::Menu* menu) {
            menu->addChild (rack::createSubmenuItem ("Theme settings", "", [] (rack::ui::Menu* menu) {
                menu->addChild (rack::createMenuLabel ("Default light theme"));
                ThemeId::forEachValue ([=] (ThemeId id) { menu->addChild (createThemeMenuItem (id.getDisplayName (), "", &pluginSettings.global_ThemeLight, id)); });

                menu->addChild (new rack::ui::MenuSeparator);
                menu->addChild (rack::createMenuLabel ("Default dark theme"));
                ThemeId::forEachValue ([=] (ThemeId id) { menu->addChild (createThemeMenuItem (id.getDisplayName (), "", &pluginSettings.global_ThemeDark, id)); });

                menu->addChild (new rack::ui::MenuSeparator);
                menu->addChild (rack::createMenuLabel ("Default emblem"));
                EmblemId::forEachValue ([=] (EmblemId id) { menu->addChild (createThemeMenuItem (id.getDisplayName (), "", &pluginSettings.global_DefaultEmblem, id)); });
            }));
        }

        virtual void createLocalStyleMenu (rack::ui::Menu* menu) {
            using rack::ui::Menu;
            using rack::createMenuLabel;
            using rack::createSubmenuItem;
            using rack::createCheckMenuItem;

            auto createThemeOverrideItem = [=] (std::string name, ThemeId themeId) {
                return createCheckMenuItem (name, "", [=] { return moduleT->theme_Override == themeId; }, [=] { setTheme (themeId); });
            };
            auto createEmblemOverrideItem = [=] (std::string name, EmblemId emblem) {
                return createCheckMenuItem (name, "", [=] { return moduleT->theme_Emblem == emblem; }, [=] { setEmblem (emblem); });
            };

            menu->addChild (createMenuLabel ("Theme"));
            menu->addChild (createThemeOverrideItem ("Default", ThemeId::getUnknown ()));
            ThemeId::forEachValue ([=] (ThemeId id) { menu->addChild (createThemeOverrideItem (id.getDisplayName (), id)); });

            menu->addChild (new rack::ui::MenuSeparator);
            menu->addChild (createMenuLabel ("Emblem"));
            menu->addChild (createEmblemOverrideItem ("Default", EmblemId::getUnknown ()));
            EmblemId::forEachValue ([=] (EmblemId id) { menu->addChild (createEmblemOverrideItem (id.getDisplayName (), id)); });
        }

        void step () override {
            callUpdateTheme ();
            callUpdateEmblem ();

            this->_ThemeHolderWidgetBase::step ();
        }

        void appendContextMenu (rack::ui::Menu* menu) override {
            using rack::ui::Menu;
            using rack::createMenuLabel;
            using rack::createSubmenuItem;
            using rack::createCheckMenuItem;

            TBase::appendContextMenu (menu);
            menu->addChild (new rack::ui::MenuSeparator);
            menu->addChild (createSubmenuItem ("Global settings", "", [&] (Menu* menu) { createPluginSettingsMenu (menu); }));
            menu->addChild (createSubmenuItem ("Local style", "", [&] (Menu* menu) { createLocalStyleMenu (menu); }));
        }
    };
}