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

#include "ResetHelper.hpp"

#include "../UI/WidgetUtils.hpp"

namespace OuroborosModules::Modules::ResetHelper {
    ResetHelperWidget::ResetHelperWidget (ResetHelperModule* module) { constructor (module, "panels/ResetHelper"); }

    void ResetHelperWidget::initializeWidget () {
        using rack::createInputCentered;
        using rack::createOutputCentered;
        using rack::createParamCentered;
        using rack::createWidget;
        using rack::math::Vec;
        using Widgets::CableJackInput;
        using Widgets::CableJackOutput;
        using Widgets::createLightCentered;
        using Widgets::EmblemWidget;
        using Widgets::GreenBlueLight;
        using Widgets::MetalKnobSmall;
        using Widgets::ResizableVCVLight;
        using Widgets::ScrewWidget;

        addChild (createWidget<ScrewWidget> (Vec ()));
        addChild (createWidget<ScrewWidget> (Vec (box.size.x, RACK_GRID_HEIGHT).minus (Vec (RACK_GRID_WIDTH))));

        emblemWidget = new Widgets::EmblemWidget (curEmblem, findNamed ("widgetLogo", Vec ()));
        addChild (emblemWidget);

        addChild (createParamCentered<MetalKnobSmall> (findNamed ("param_TrigLen", Vec ()), moduleT, ResetHelperModule::PARAM_TRIGGER_LENGTH));
        addChild (createParamCentered<MetalKnobSmall> (findNamed ("param_MergeTime", Vec ()), moduleT, ResetHelperModule::PARAM_MERGE_TIME));

        forEachMatched ("input_Reset(\\d+)", [&] (std::vector<std::string> captures, Vec pos) {
            auto i = stoi (captures [0]) - 1;
            if (i < 0 || i > ResetHelperModule::InputCount)
                return LOG_WARN (FMT_STRING ("ResetHelper panel has invalid trigger input #{}"), i);
            addInput (createInputCentered<CableJackInput> (pos, moduleT, ResetHelperModule::INPUT_RESET + i));
        });

        forEachMatched ("light_ResetInput(\\d+)", [&] (std::vector<std::string> captures, Vec pos) {
            auto i = stoi (captures [0]) - 1;
            if (i < 0 || i > ResetHelperModule::InputCount)
                return LOG_WARN (FMT_STRING ("ResetHelper panel has invalid trigger light #{}"), i);
            addChild (createLightCentered<ResizableVCVLight<GreenBlueLight>> (pos, moduleT, ResetHelperModule::LIGHT_INPUT + i * 2, 4.5f));
        });

        addOutput (createOutputCentered<CableJackOutput> (findNamed ("output_ResetTrigger", Vec ()), moduleT, ResetHelperModule::OUTPUT_RESET));
        addChild (createLightCentered<ResizableVCVLight<GreenBlueLight>> (findNamed ("light_ResetTrigger", Vec ()), moduleT, ResetHelperModule::LIGHT_OUTPUT, 4.5f));
    }

    void ResetHelperWidget::onChangeEmblem (EmblemId emblemId) {
        _WidgetBase::onChangeEmblem (emblemId);
        emblemWidget->setEmblem (emblemId);
    }
}