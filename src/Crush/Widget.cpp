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

#include "Crush.hpp"

#include "../UI/WidgetUtils.hpp"

#include <fmt/format.h>

namespace OuroborosModules::Modules::Crush {
    CrushWidget::CrushWidget (CrushModule* module) { constructor (module, "panels/Crush"); }

    void CrushWidget::initializeWidget () {
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
        using Widgets::TrimmerKnob;

        addChild (createWidget<ScrewWidget> (Vec ()));
        addChild (createWidget<ScrewWidget> (Vec (box.size.x, RACK_GRID_HEIGHT).minus (Vec (RACK_GRID_WIDTH))));

        emblemWidget = new Widgets::EmblemWidget (curEmblem, findNamed ("widgetLogo", Vec ()));
        addChild (emblemWidget);

        // Inputs
        addInput (createInputCentered<CableJackInput> (findNamed ("input_Signal", Vec ()), moduleT, CrushModule::INPUT_SIGNAL))
            ->enableConnectToMixmaster ();

        addInput (createInputCentered<CableJackInput> (findNamed ("input_AmountCV", Vec ()), moduleT, CrushModule::INPUT_AMOUNT_CV));
        addInput (createInputCentered<CableJackInput> (findNamed ("input_TargetLevel", Vec ()), moduleT, CrushModule::INPUT_TARGET_LEVEL));

        // Outputs
        addOutput (createOutputCentered<CableJackOutput> (findNamed ("output_Signal", Vec ()), moduleT, CrushModule::OUTPUT_SIGNAL))
            ->enableConnectToMixmaster ()->enableConnectToNeighbor ();
        addOutput (createOutputCentered<CableJackOutput> (findNamed ("output_Envelope", Vec ()), moduleT, CrushModule::OUTPUT_ENVELOPE));

        // Params
        addChild (createParamCentered<MetalKnobSmall> (findNamed ("param_Amount", Vec ()), moduleT, CrushModule::PARAM_AMOUNT));
        addChild (createParamCentered<TrimmerKnob> (findNamed ("param_AmountCVAtten", Vec ()), moduleT, CrushModule::PARAM_AMOUNT_CV_ATTEN));

        addChild (createParamCentered<MetalKnobSmall> (findNamed ("param_TargetLevel", Vec ()), moduleT, CrushModule::PARAM_TARGET_LEVEL));

        addChild (createParamCentered<TrimmerKnob> (findNamed ("param_FilterTime", Vec ()), moduleT, CrushModule::PARAM_FILTER_TIME));

    }

    void CrushWidget::onChangeEmblem (EmblemId emblemId) {
        _WidgetBase::onChangeEmblem (emblemId);
        emblemWidget->setEmblem (emblemId);
    }
}