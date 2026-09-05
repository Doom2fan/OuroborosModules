/*
 *  OuroborosModules
 *  Copyright (C) 2026 Chronos "phantombeta" Ouroboros
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

#include "Thrum.hpp"

#include "../UI/WidgetUtils.hpp"

#include <fmt/format.h>

namespace OuroborosModules::Modules::Thrum {
    ThrumWidget::ThrumWidget (ThrumModule* module) { constructor (module, "panels/Thrum"); }

    void ThrumWidget::initializeWidget () {
        using rack::createInputCentered;
        using rack::createOutputCentered;
        using rack::createParamCentered;
        using rack::createWidget;
        using rack::math::Vec;
        using Widgets::CableJackInput;
        using Widgets::CableJackOutput;
        using Widgets::createLightCentered;
        using Widgets::EmblemWidget;
        using Widgets::MetalKnobSmall;
        using Widgets::ScrewWidget;

        addChild (createWidget<ScrewWidget> (Vec ()));
        addChild (createWidget<ScrewWidget> (Vec (box.size.x, RACK_GRID_HEIGHT).minus (Vec (RACK_GRID_WIDTH))));

        emblemWidget = new Widgets::EmblemWidget (curEmblem, findNamed ("widgetLogo", Vec ()));
        addChild (emblemWidget);

        constexpr float knobSize = 24;
        constexpr float knobSpacingX = 8;
        constexpr float knobSpacingY = 8;
        float maxX = box.size.x;
        float basePosX = 8;
        float posX = basePosX;
        float posY = 32;
        for (int i = 0; i < ThrumModule::NUM_PARAMS; i++) {
            if (posX + knobSize > maxX) {
                posY += knobSize + knobSpacingY;
                posX = basePosX;
            }

            auto knobPos = Vec (posX, posY);
            addChild (rack::createParam<MetalKnobSmall> (knobPos, moduleT, i));
            posX += knobSize + knobSpacingX;
        }
        posY += knobSize + knobSpacingY;

        // Inputs
        addInput (rack::createInput<CableJackInput> (Vec (8, posY), moduleT, ThrumModule::INPUT_TRIGGER));
        addInput (rack::createInput<CableJackInput> (Vec (8, posY + knobSpacingY + knobSize), moduleT, ThrumModule::INPUT_VELOCITY));

        // Outputs
        addOutput (rack::createOutput<CableJackOutput> (Vec (40, posY), moduleT, ThrumModule::OUTPUT_SIGNAL));
    }

    void ThrumWidget::onChangeEmblem (EmblemId emblemId) {
        _WidgetBase::onChangeEmblem (emblemId);
        emblemWidget->setEmblem (emblemId);
    }
}