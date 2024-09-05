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

#include "../CableHandler.hpp"
#include "../ModuleBase.hpp"
#include "../PluginDef.hpp"
#include "../UI/ImageWidget.hpp"
#include "../UI/WidgetBase.hpp"
#include "CableColorManager.hpp"
#include "ColorDisplayWidget.hpp"

#include <memory>

namespace OuroborosModules {
namespace CableColorModule {
    struct KeyContainer;

    enum class CenterEmblem {
        Default = -1,

        False = 0,
        True = 1,
    };

    struct CableColorModule : ModuleBase {
        std::shared_ptr<CableColorManager> colorManager = nullptr;
        CenterEmblem centerEmblem = CenterEmblem::Default;
        bool updateEmblem = false;

        CableColorModule ();

        bool checkMaster ();

        json_t* dataToJson () override;
        void dataFromJson (json_t* rootJ) override;
    };

    struct CableColorModuleWidget : Widgets::ModuleWidgetBase<CableColorModuleWidget, CableColorModule> {
        friend KeyContainer;

      private:
        ColorDisplayWidget* colorDisplayWidget;
        Widgets::ImageWidget* emblemWidget = nullptr;
        KeyContainer* keyContainer = nullptr;

      public:
        CableColorModuleWidget (CableColorModule* module);
        ~CableColorModuleWidget ();

      protected:
        void initializeWidget () override;

        void step () override;
        void update ();

        void updateEmblem (ThemeId themeId, EmblemId emblemId);
        void onChangeTheme (ThemeId themeId) override;
        void onChangeEmblem (EmblemId emblemId) override;

        void createLocalStyleMenu (rack::ui::Menu* menu) override;
        void createPluginSettingsMenu (rack::ui::Menu* menu) override;
        void createCollectionsMenu (rack::ui::Menu* menu);
        void createReplacePatchCablesMenu (rack::ui::Menu* menu);
        void appendContextMenu (rack::ui::Menu* menu) override;
    };

    struct OverlayWindow;

    struct KeyContainer : rack::widget::TransparentWidget {
        friend CableColorModuleWidget;

      private:
        CableColorModuleWidget* moduleWidget = nullptr;
        OverlayWindow* overlayWindow;

      public:
        KeyContainer (CableColorModuleWidget* moduleWidget);
        ~KeyContainer ();

        void step () override;

        void displayMessage (std::string message);
        void closeMessage ();

        bool checkLearningMode (const rack::event::Base& e);
        void onButton (const rack::event::Button& e) override;
        void onHoverKey (const rack::event::HoverKey& e) override;
        void onHoverText (const rack::event::HoverText& e) override;
        void onHoverScroll (const rack::event::HoverScroll& e) override;
    };
}
}