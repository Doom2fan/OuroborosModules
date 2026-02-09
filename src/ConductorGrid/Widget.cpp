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

#include "ConductorGrid.hpp"

#include "../UI/WidgetUtils.hpp"

#include <fmt/format.h>

namespace OuroborosModules::Modules::Conductor {
    ConductorGridWidget::ConductorGridWidget (ConductorGridModule* module) { constructor (module, "panels/ConductorGrid"); }

    struct ConductorGridPad : rack_themer::widgets::SvgSwitch {
        using LightColor = Widgets::RedBlueLight;
        struct ButtonLight : rack::componentlibrary::RectangleLight<LightColor> {
            ButtonLight () {
                this->borderColor = rack::color::BLACK_TRANSPARENT;
                this->bgColor = rack::color::BLACK_TRANSPARENT;
                this->box.size = rack::math::Vec (100);
            }
        };

        rack::app::ModuleLightWidget* light;

        ConductorGridPad () {
            momentary = true;
            addFrame (Theme::getSvg ("components/ConductorGridPad"));

            light = new ButtonLight;
            // Move center of light to center of box
            light->box.pos = box.size.div (2).minus (light->box.size.div (2));
            addChild (light);

            shadow->hide ();
        }

        rack::app::ModuleLightWidget* getLight () { return light; }

        ConductorGridPad (const ConductorGridPad& x) = delete;
        void operator= (const ConductorGridPad& x) = delete;
    };

    struct ConductorGridPadSmaller : rack_themer::widgets::SvgSwitch {
        using LightColor = rack::componentlibrary::BlueLight;
        struct ButtonLight : rack::componentlibrary::RectangleLight<LightColor> {
            ButtonLight () {
                this->borderColor = rack::color::BLACK_TRANSPARENT;
                this->bgColor = rack::color::BLACK_TRANSPARENT;
                this->box.size = rack::math::Vec (45);
            }
        };

        rack::app::ModuleLightWidget* light;

        ConductorGridPadSmaller () {
            momentary = true;
            addFrame (Theme::getSvg ("components/ConductorGridPadSmaller"));

            light = new ButtonLight;
            // Move center of light to center of box
            light->box.pos = box.size.div (2).minus (light->box.size.div (2));
            addChild (light);

            shadow->hide ();
        }

        rack::app::ModuleLightWidget* getLight () { return light; }

        ConductorGridPadSmaller (const ConductorGridPadSmaller& x) = delete;
        void operator= (const ConductorGridPadSmaller& x) = delete;
    };

    void ConductorGridWidget::initializeWidget () {
        using rack::createInputCentered;
        using rack::createLightParamCentered;
        using rack::createParamCentered;
        using rack::math::Vec;
        using Widgets::CableJackInput;
        using Widgets::createLightCentered;
        using Widgets::createWidget;
        using Widgets::EmblemWidget;
        using Widgets::MetalKnobSmall;
        using Widgets::ResizableVCVLight;
        using Widgets::ScrewWidget;

        using GridModule = ConductorGridModule;

        addChild (createWidget<ScrewWidget> (Vec ()));
        addChild (createWidget<ScrewWidget> (Vec (box.size.x, RACK_GRID_HEIGHT).minus (Vec (RACK_GRID_WIDTH))));

        emblemWidget = new Widgets::EmblemWidget (curEmblem, findNamed ("widgetLogo", Vec ()));
        addChild (emblemWidget);

        // Pads.
        forEachMatched ("param_Pad(\\d+)", [&] (std::vector<std::string> captures, Vec pos) {
            auto i = stoi (captures [0]) - 1;
            if (i < 0 || i > GridModule::PadCount)
                return LOG_WARN (FMT_STRING ("ConductorGrid panel has invalid pad #{}"), i);

            auto padButton = createLightParamCentered<ConductorGridPad> (pos, moduleT, GridModule::PARAM_PAD_BUTTON + i, GridModule::LIGHT_PAD_BUTTON + i * 2);
            addChild (padButton);
        });

        // Page display and buttons.
        auto pageDisplayBox = findNamedBox ("widget_PageDisplay", rack::math::Rect ());
        pageDisplay = createWidget<LedNumberDisplay> (pageDisplayBox.pos, pageDisplayBox.size, 32.f, 3, [&] {
            return moduleT != nullptr ? moduleT->curPage + 1 : 5;
        });
        addChild (pageDisplay);

        addChild (createLightParamCentered<ConductorGridPadSmaller> (findNamed ("param_PageDown", Vec ()), moduleT, GridModule::PARAM_PAGE_DOWN_BUTTON, GridModule::LIGHT_PAGE_DOWN_BUTTON));
        addChild (createLightParamCentered<ConductorGridPadSmaller> (findNamed ("param_PageUp", Vec ()), moduleT, GridModule::PARAM_PAGE_UP_BUTTON, GridModule::LIGHT_PAGE_UP_BUTTON));

        // Queue display.
        auto queueDisplayBox = findNamedBox ("widget_QueueDisplay", rack::math::Rect ());
        queueDisplay = createWidget<LedNumberDisplay> (queueDisplayBox.pos, queueDisplayBox.size, 32.f, 3, [&] {
            return moduleT != nullptr ? moduleT->queuedPattern + 1 : 0;
        });
        queueDisplay->disabled = moduleT == nullptr;
        addChild (queueDisplay);
    }

    void ConductorGridWidget::step () {
        _WidgetBase::step ();

        if (moduleT == nullptr)
            return;

        pageDisplay->disabled = !moduleT->enabled;
        queueDisplay->disabled = !moduleT->enabled || moduleT->queuedPattern < 0;
    }

    void ConductorGridWidget::onChangeEmblem (EmblemId emblemId) {
        _WidgetBase::onChangeEmblem (emblemId);
        emblemWidget->setEmblem (emblemId);
    }
}