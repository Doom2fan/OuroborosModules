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

#include "Diffuse.hpp"

#include "../UI/WidgetUtils.hpp"

#include <fmt/format.h>

namespace OuroborosModules::Modules::Diffuse {
    DiffuseWidget::DiffuseWidget (DiffuseModule* module) { constructor (module, "panels/Diffuse"); }

    void DiffuseWidget::initializeWidget () {
        using rack::createInputCentered;
        using rack::createOutputCentered;
        using rack::createParamCentered;
        using rack::createWidget;
        using rack::math::Vec;
        using Widgets::CableJackInput;
        using Widgets::CableJackOutput;
        using Widgets::createLightCentered;
        using Widgets::EmblemWidget;
        using Widgets::MetalKnobHuge;
        using Widgets::MetalKnobSmall;
        using Widgets::ScrewWidget;
        using Widgets::TrimmerKnob;

        addChild (createWidget<ScrewWidget> (Vec ()));
        addChild (createWidget<ScrewWidget> (Vec (box.size.x, RACK_GRID_HEIGHT).minus (Vec (RACK_GRID_WIDTH))));

        emblemWidget = new Widgets::EmblemWidget (curEmblem, findNamed ("widgetLogo", Vec ()));
        addChild (emblemWidget);

        // Inputs
        addInput (createInputCentered<CableJackInput> (findNamed ("input_Signal", Vec ()), moduleT, DiffuseModule::INPUT_SIGNAL));
        addInput (createInputCentered<CableJackInput> (findNamed ("input_VOct", Vec ()), moduleT, DiffuseModule::INPUT_VOCT));

        addInput (createInputCentered<CableJackInput> (findNamed ("input_FreqCV", Vec ()), moduleT, DiffuseModule::INPUT_FREQ_CV));
        addInput (createInputCentered<CableJackInput> (findNamed ("input_GainCV", Vec ()), moduleT, DiffuseModule::INPUT_GAIN_CV));
        addInput (createInputCentered<CableJackInput> (findNamed ("input_MixCV", Vec ()), moduleT, DiffuseModule::INPUT_MIX_CV));

        // Outputs
        addOutput (createOutputCentered<CableJackOutput> (findNamed ("output_Wet", Vec ()), moduleT, DiffuseModule::OUTPUT_WET));
        addOutput (createOutputCentered<CableJackOutput> (findNamed ("output_Mix", Vec ()), moduleT, DiffuseModule::OUTPUT_MIX));

        // Params
        addChild (createParamCentered<MetalKnobHuge> (findNamed ("param_Freq", Vec ()), moduleT, DiffuseModule::PARAM_FREQ));
        addChild (createParamCentered<MetalKnobHuge> (findNamed ("param_Gain", Vec ()), moduleT, DiffuseModule::PARAM_GAIN));
        addChild (createParamCentered<MetalKnobHuge> (findNamed ("param_Mix", Vec ()), moduleT, DiffuseModule::PARAM_MIX));

        addChild (createParamCentered<TrimmerKnob> (findNamed ("param_FreqCVAtten", Vec ()), moduleT, DiffuseModule::PARAM_FREQ_CV_ATTEN));
        addChild (createParamCentered<TrimmerKnob> (findNamed ("param_GainCVAtten", Vec ()), moduleT, DiffuseModule::PARAM_GAIN_CV_ATTEN));
        addChild (createParamCentered<TrimmerKnob> (findNamed ("param_MixCVAtten", Vec ()), moduleT, DiffuseModule::PARAM_MIX_CV_ATTEN));
    }

    void DiffuseWidget::onChangeEmblem (EmblemId emblemId) {
        _WidgetBase::onChangeEmblem (emblemId);
        emblemWidget->setEmblem (emblemId);
    }
}