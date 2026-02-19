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

#include "Warp.hpp"

#include "../UI/WidgetUtils.hpp"

#include <fmt/format.h>

namespace OuroborosModules::Modules::Warp {
    WarpWidget::WarpWidget (WarpModule* module) { constructor (module, "panels/Warp"); }

    void WarpWidget::initializeWidget () {
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
        addInput (createInputCentered<CableJackInput> (findNamed ("input_Signal", Vec ()), moduleT, WarpModule::INPUT_SIGNAL))
            ->enableConnectToMixmaster ();
        addInput (createInputCentered<CableJackInput> (findNamed ("input_Modulator", Vec ()), moduleT, WarpModule::INPUT_MODULATOR))
            ->enableConnectToMixmaster ();

        addInput (createInputCentered<CableJackInput> (findNamed ("input_AmountCV", Vec ()), moduleT, WarpModule::INPUT_AMOUNT_CV));
        addInput (createInputCentered<CableJackInput> (findNamed ("input_BiasCV", Vec ()), moduleT, WarpModule::INPUT_BIAS_CV));

        // Outputs
        addOutput (createOutputCentered<CableJackOutput> (findNamed ("output_Signal", Vec ()), moduleT, WarpModule::OUTPUT_SIGNAL))
            ->enableConnectToMixmaster ()->enableConnectToNeighbor ();

        // Params
        addChild (createParamCentered<MetalKnobSmall> (findNamed ("param_Amount", Vec ()), moduleT, WarpModule::PARAM_AMOUNT));
        addChild (createParamCentered<TrimmerKnob> (findNamed ("param_AmountCVAtten", Vec ()), moduleT, WarpModule::PARAM_AMOUNT_CV_ATTEN));

        addChild (createParamCentered<MetalKnobSmall> (findNamed ("param_Bias", Vec ()), moduleT, WarpModule::PARAM_BIAS));
        addChild (createParamCentered<TrimmerKnob> (findNamed ("param_BiasCVAtten", Vec ()), moduleT, WarpModule::PARAM_BIAS_CV_ATTEN));
    }

    void WarpWidget::onChangeEmblem (EmblemId emblemId) {
        _WidgetBase::onChangeEmblem (emblemId);
        emblemWidget->setEmblem (emblemId);
    }

    void WarpWidget::appendContextMenu (rack::ui::Menu* menu) {
        using rack::ui::Menu;

        _WidgetBase::appendContextMenu (menu);

        // Oversampling options
        menu->addChild (new rack::ui::MenuSeparator);
        menu->addChild (rack::createSubmenuItem ("Oversampling", "", [=] (Menu* menu) {
            auto curOversample = static_cast<uint32_t> (moduleT->params [WarpModule::PARAM_OVERSAMPLE].getValue ());
            for (uint32_t accum = 1; accum <= WarpModule::MaxOversample; accum *= 2) {
                auto label = accum > 1 ? fmt::format (FMT_STRING ("{}x"), accum) : "Off";
                auto isCurrent = accum == curOversample;

                menu->addChild (rack::createCheckMenuItem (label, "",
                    [=] { return isCurrent; },
                    [=] {
                        createContextMenuHistory<uint32_t> ("Set Warp oversampling factor", [=] (WarpModule* module, uint32_t value) {
                            APP->engine->setParamValue (module, WarpModule::PARAM_OVERSAMPLE, value);
                        }, curOversample, accum);
                    }
                ));
            }
        }));
    }
}