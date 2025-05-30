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

#include "Junction.hpp"

#include <fmt/format.h>

namespace OuroborosModules::Modules::Junction {
    JunctionWidget::JunctionWidget (JunctionModule* module) { constructor (module, "panels/Junction"); }

    void JunctionWidget::initializeWidget () {
        using rack::createInputCentered;
        using rack::createOutputCentered;
        using rack::createParamCentered;
        using rack::createWidget;
        using rack::math::Vec;
        using Widgets::CableJackInput;
        using Widgets::CableJackOutput;
        using Widgets::EmblemWidget;
        using Widgets::ScrewWidget;

        addChild (createWidget<ScrewWidget> (Vec ()));
        addChild (createWidget<ScrewWidget> (Vec (box.size.x, RACK_GRID_HEIGHT).minus (Vec (RACK_GRID_WIDTH))));

        emblemWidget = new Widgets::EmblemWidget (curEmblem, findNamed ("widgetLogo", Vec ()));
        addChild (emblemWidget);

        forEachMatched ("input_Signal(\\d+)", [&] (std::vector<std::string> captures, Vec pos) {
            auto i = stoi (captures [0]) - 1;
            if (i < 0 || i > JunctionModule::SwitchCount)
                return LOG_WARN (FMT_STRING ("Junction panel has invalid signal input #{}"), i);

            addInput (createInputCentered<CableJackInput> (pos, module, JunctionModule::INPUT_SIGNAL + i));
        });

        forEachMatched ("param_SignalDest(\\d+)", [&] (std::vector<std::string> captures, Vec pos) {
            auto i = stoi (captures [0]) - 1;
            if (i < 0 || i > JunctionModule::SwitchCount)
                return LOG_WARN (FMT_STRING ("Junction panel has invalid destination knob #{}"), i);

            addChild (createParamCentered<Widgets::MetalKnobSmall> (pos, module, JunctionModule::PARAM_SWITCH + i));
        });

        addOutput (createOutputCentered<CableJackOutput> (findNamed ("output_A", Vec ()), module, JunctionModule::OUTPUT_SIGNAL));
        addOutput (createOutputCentered<CableJackOutput> (findNamed ("output_B", Vec ()), module, JunctionModule::OUTPUT_SIGNAL + 1));
    }

    void JunctionWidget::onChangeEmblem (EmblemId emblemId) {
        _WidgetBase::onChangeEmblem (emblemId);
        emblemWidget->setEmblem (emblemId);
    }

    void JunctionWidget::appendContextMenu (rack::ui::Menu* menu) {
        _WidgetBase::appendContextMenu (menu);

        menu->addChild (new rack::ui::MenuSeparator);
        menu->addChild (rack::createBoolPtrMenuItem ("Determine polyphony from selected inputs", "", &module->polyOnDemand));
        menu->addChild (rack::createBoolPtrMenuItem ("Clamp while summing", "", &module->clampWhileSumming));
    }
}