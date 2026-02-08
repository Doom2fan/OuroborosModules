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

#include "Conductor.hpp"

#include "../UI/WidgetUtils.hpp"

#include <fmt/format.h>

namespace OuroborosModules::Modules::Conductor {
    ConductorWidget::ConductorWidget (ConductorModule* module) { constructor (module, "panels/Conductor"); }

    void ConductorWidget::initializeWidget () {
        using rack::createInputCentered;
        using rack::createLightParamCentered;
        using rack::createOutputCentered;
        using rack::createParamCentered;
        using rack::math::Vec;
        using Widgets::CableJackInput;
        using Widgets::CableJackOutput;
        using Widgets::createWidget;
        using Widgets::EmblemWidget;
        using Widgets::MetalKnobSmall;
        using Widgets::ScrewWidget;

        using LightButton = Widgets::LightButton<>;

        addChild (createWidget<ScrewWidget> (Vec ()));
        addChild (createWidget<ScrewWidget> (Vec (box.size.x, RACK_GRID_HEIGHT).minus (Vec (RACK_GRID_WIDTH))));

        emblemWidget = new Widgets::EmblemWidget (curEmblem, findNamed ("widgetLogo", Vec ()));
        addChild (emblemWidget);

        // Params
        addChild (createParamCentered<MetalKnobSmall> (findNamed ("param_PtnCount", Vec ()), moduleT, ConductorModule::PARAM_PATTERN_COUNT));

        addChild (createParamCentered<MetalKnobSmall> (findNamed ("param_ResetPtn", Vec ()), moduleT, ConductorModule::PARAM_RESET_PATTERN));
        addChild (createLightParamCentered<LightButton> (findNamed ("param_ResetPtnButton", Vec ()), moduleT, ConductorModule::PARAM_RESET_PATTERN_BUTTON, ConductorModule::LIGHT_RESET_PATTERN_BUTTON));

        addChild (createParamCentered<MetalKnobSmall> (findNamed ("param_SeqPtnMax", Vec ()), moduleT, ConductorModule::PARAM_SEQ_MAX_PATTERNS));
        addChild (createParamCentered<MetalKnobSmall> (findNamed ("param_SeqCVMax", Vec ()), moduleT, ConductorModule::PARAM_SEQ_MAX_CV));

        addChild (createLightParamCentered<LightButton> (findNamed ("param_AdvanceButton", Vec ()), moduleT, ConductorModule::PARAM_MANUAL_ADVANCE_BUTTON, ConductorModule::LIGHT_ADVANCE_BUTTON));
        addChild (createLightParamCentered<LightButton> (findNamed ("param_ResetButton", Vec ()), moduleT, ConductorModule::PARAM_MANUAL_RESET_BUTTON, ConductorModule::LIGHT_RESET_BUTTON));

        addChild (createParamCentered<MetalKnobSmall> (findNamed ("param_ManualSet", Vec ()), moduleT, ConductorModule::PARAM_MANUAL_SET));
        addChild (createLightParamCentered<LightButton> (findNamed ("param_ManualSetButton", Vec ()), moduleT, ConductorModule::PARAM_MANUAL_SET_BUTTON, ConductorModule::LIGHT_MANUAL_SET_BUTTON));

        // Inputs
        addInput (createInputCentered<CableJackInput> (findNamed ("input_Clock", Vec ()), moduleT, ConductorModule::INPUT_CLOCK));
        addInput (createInputCentered<CableJackInput> (findNamed ("input_Advance", Vec ()), moduleT, ConductorModule::INPUT_ADVANCE));
        addInput (createInputCentered<CableJackInput> (findNamed ("input_Reset", Vec ()), moduleT, ConductorModule::INPUT_RESET));

        // Outputs
        addOutput (createOutputCentered<CableJackOutput> (findNamed ("output_Clock", Vec ()), moduleT, ConductorModule::OUTPUT_CLOCK));
        addOutput (createOutputCentered<CableJackOutput> (findNamed ("output_Reset", Vec ()), moduleT, ConductorModule::OUTPUT_RESET));
        addOutput (createOutputCentered<CableJackOutput> (findNamed ("output_PatternCV", Vec ()), moduleT, ConductorModule::OUTPUT_PATTERN));

        // Display
        auto displayBox = findNamedBox ("widget_Display", rack::math::Rect ());
        auto displayWidget = createWidget<LedNumberDisplay> (displayBox.pos, displayBox.size, 32.f, 3, [&] {
            return moduleT != nullptr ? moduleT->currentPattern + 1 : 16;
        });
        addChild (displayWidget);
    }

    void ConductorWidget::onChangeEmblem (EmblemId emblemId) {
        _WidgetBase::onChangeEmblem (emblemId);
        emblemWidget->setEmblem (emblemId);
    }

    void ConductorWidget::appendContextMenu (rack::ui::Menu* menu) {
        using rack::createBoolPtrMenuItem;
        using rack::createMenuLabel;
        using rack::ui::Menu;

        _WidgetBase::appendContextMenu (menu);

        menu->addChild (new rack::ui::MenuSeparator);

        // Sequencer options.
        menu->addChild (createMenuLabel ("Sequencer options"));
        auto voltageOffsetSlider = new rack::ui::Slider ();
        voltageOffsetSlider->quantity = moduleT->getParamQuantity (ConductorModule::PARAM_PATTERN_OFFSET_VOLTAGE);
        voltageOffsetSlider->box.size.x = 200.f;
        menu->addChild (voltageOffsetSlider);

        // Clock options.
        menu->addChild (createMenuLabel ("Clock options"));
        auto clockDelaySlider = new rack::ui::Slider ();
        clockDelaySlider->quantity = moduleT->getParamQuantity (ConductorModule::PARAM_CLOCK_DELAY);
        clockDelaySlider->box.size.x = 200.f;
        menu->addChild (clockDelaySlider);
        menu->addChild (createBoolPtrMenuItem ("Ignore first clock within 1ms after reset", "", &moduleT->resetIgnoreFirstClock));
    }
}