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

#include "ConductorExternal.hpp"

#include "../UI/WidgetUtils.hpp"

#include <fmt/format.h>

namespace OuroborosModules::Modules::Conductor {
    ConductorExternalWidget::ConductorExternalWidget (ConductorExternalModule* module) { constructor (module, "panels/ConductorExternal"); }

    void ConductorExternalWidget::initializeWidget () {
        using rack::createInputCentered;
        using rack::createLightParamCentered;
        using rack::createParamCentered;
        using rack::componentlibrary::GreenLight;
        using rack::math::Vec;
        using Widgets::CableJackInput;
        using Widgets::createLightCentered;
        using Widgets::createWidget;
        using Widgets::EmblemWidget;
        using Widgets::MetalKnobSmall;
        using Widgets::ResizableVCVLight;
        using Widgets::ScrewWidget;

        using ExternalModule = ConductorExternalModule;
        using LightButtonRedBlue = Widgets::LightButton<Widgets::RedBlueLight>;

        addChild (createWidget<ScrewWidget> (Vec ()));
        addChild (createWidget<ScrewWidget> (Vec (box.size.x, RACK_GRID_HEIGHT).minus (Vec (RACK_GRID_WIDTH))));

        emblemWidget = new Widgets::EmblemWidget (curEmblem, findNamed ("widgetLogo", Vec ()));
        addChild (emblemWidget);

        // Params
        addChild (createParamCentered<MetalKnobSmall> (findNamed ("param_Mode", Vec ()), moduleT, ExternalModule::PARAM_MODE));
        addChild (createLightParamCentered<LightButtonRedBlue> (findNamed ("param_MapButton", Vec ()), moduleT, ExternalModule::PARAM_MAP_BUTTON, ExternalModule::LIGHT_MAP_BUTTON));

        // Inputs
        addInput (createInputCentered<CableJackInput> (findNamed ("input_CV1", Vec ()), moduleT, ExternalModule::INPUT_CV1));
        addInput (createInputCentered<CableJackInput> (findNamed ("input_CV2", Vec ()), moduleT, ExternalModule::INPUT_CV2));
        addInput (createInputCentered<CableJackInput> (findNamed ("input_Gate", Vec ()), moduleT, ExternalModule::INPUT_GATE));

        // Lights
        addChild (createLightCentered<ResizableVCVLight<GreenLight>> (findNamed ("light_CV1", Vec ()), moduleT, ExternalModule::LIGHT_CV1_ENABLED, 5.f));
        addChild (createLightCentered<ResizableVCVLight<GreenLight>> (findNamed ("light_CV2", Vec ()), moduleT, ExternalModule::LIGHT_CV2_ENABLED, 5.f));

        // Displays
        auto selDisplayBox = findNamedBox ("widget_SelDisplay", rack::math::Rect ());
        selectionDisplay = createWidget<LedNumberDisplay> (selDisplayBox.pos, selDisplayBox.size, 32.f, 3, [&] {
            return moduleT != nullptr ? moduleT->selectedPattern + 1 : 5;
        });
        addChild (selectionDisplay);

        auto queueDisplayBox = findNamedBox ("widget_QueueDisplay", rack::math::Rect ());
        queueDisplay = createWidget<LedNumberDisplay> (queueDisplayBox.pos, queueDisplayBox.size, 32.f, 3, [&] {
            return moduleT != nullptr ? moduleT->queuedPattern + 1 : 0;
        });
        queueDisplay->disabled = moduleT == nullptr;
        addChild (queueDisplay);
    }

    void ConductorExternalWidget::step () {
        _WidgetBase::step ();

        if (moduleT == nullptr)
            return;

        if (selectionDisplay != nullptr)
            selectionDisplay->disabled = !moduleT->enabled;
        if (queueDisplay != nullptr)
            queueDisplay->disabled = !moduleT->enabled || moduleT->queuedPattern < 0;
    }

    void ConductorExternalWidget::onChangeEmblem (EmblemId emblemId) {
        _WidgetBase::onChangeEmblem (emblemId);
        emblemWidget->setEmblem (emblemId);
    }

    void ConductorExternalWidget::appendContextMenu (rack::ui::Menu* menu) {
        using rack::createBoolPtrMenuItem;
        using rack::createMenuItem;
        using rack::createMenuLabel;
        using rack::createSubmenuItem;
        using rack::ui::Menu;

        _WidgetBase::appendContextMenu (menu);

        menu->addChild (new rack::ui::MenuSeparator);
        menu->addChild (createMenuItem ("Clear mappings", "", [=] {
            moduleT->noteMapState.clearMappings ();
        }));
    }
}