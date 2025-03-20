/*
 *  OuroborosModules
 *  Copyright (C) 2024 Chronos "phantombeta" Ouroboros
 *  Copyright (C) 2016-2023 VCV
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

#include "Branch.hpp"

#include <fmt/format.h>

namespace OuroborosModules::Modules::Branch {
    BranchWidget::BranchWidget (BranchModule* module) { constructor (module, "panels/Branch"); }

    void BranchWidget::initializeWidget () {
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

        addInput (createInputCentered<CableJackInput> (findNamed ("input_A", Vec ()), module, BranchModule::INPUT_A));
        addInput (createInputCentered<CableJackInput> (findNamed ("input_B", Vec ()), module, BranchModule::INPUT_B));

        forEachMatched ("output_Destination(\\d+)", [&] (std::vector<std::string> captures, Vec pos) {
            auto i = stoi (captures [0]) - 1;
            if (i < 0 || i > BranchModule::SwitchCount)
                return LOG_WARN (FMT_STRING ("Branch panel has invalid destination output #{}"), i);

            addOutput (createOutputCentered<CableJackOutput> (pos, module, BranchModule::OUTPUT_DESTINATION + i));
        });

        forEachMatched ("param_DestinationSource(\\d+)", [&] (std::vector<std::string> captures, Vec pos) {
            auto i = stoi (captures [0]) - 1;
            if (i < 0 || i > BranchModule::SwitchCount)
                return LOG_WARN (FMT_STRING ("Branch panel has invalid source knob #{}"), i);

            addChild (createParamCentered<Widgets::MetalKnobSmall> (pos, module, BranchModule::PARAM_SWITCH + i));
        });
    }

    void BranchWidget::onChangeEmblem (EmblemId emblemId) {
        _WidgetBase::onChangeEmblem (emblemId);
        emblemWidget->setEmblem (emblemId);
    }

    void BranchWidget::appendContextMenu (rack::ui::Menu* menu) {
        _WidgetBase::appendContextMenu (menu);

        menu->addChild (new rack::ui::MenuSeparator);
        menu->addChild (rack::createBoolPtrMenuItem ("Determine polyphony from selected source", "", &module->polyOnDemand));
    }
}