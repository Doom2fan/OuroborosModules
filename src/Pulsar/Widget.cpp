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

#include "Pulsar.hpp"

#include "../UI/WidgetUtils.hpp"

#include <fmt/format.h>

namespace OuroborosModules::Modules::Pulsar {
    PulsarWidget::PulsarWidget (PulsarModule* module) { constructor (module, "panels/Pulsar"); }

    void PulsarWidget::initializeWidget () {
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
        for (int i = 0; i < PulsarModule::NUM_PARAMS; i++) {
            if (posX + knobSize > maxX) {
                posY += knobSize + knobSpacingY;
                posX = basePosX;
            }

            auto knobPos = Vec (posX, posY);
            addChild (rack::createParam<MetalKnobSmall> (knobPos, moduleT, i));
            posX += knobSize + knobSpacingX;
        }
        posY += knobSize + knobSpacingY;
        posX = basePosX;

        for (int i = 0; i < PulsarModule::NUM_INPUTS; i++) {
            if (posX + knobSize > maxX) {
                posY += knobSize + knobSpacingY;
                posX = basePosX;
            }

            auto knobPos = Vec (posX, posY);
            addChild (rack::createInput<CableJackInput> (knobPos, moduleT, i));
            posX += knobSize + knobSpacingX;
        }
        posY += knobSize + knobSpacingY;
        posX = basePosX;

        for (int i = 0; i < PulsarModule::NUM_OUTPUTS; i++) {
            if (posX + knobSize > maxX) {
                posY += knobSize + knobSpacingY;
                posX = basePosX;
            }

            auto knobPos = Vec (posX, posY);
            addChild (rack::createOutput<CableJackOutput> (knobPos, moduleT, i));
            posX += knobSize + knobSpacingX;
        }
        posY += knobSize + knobSpacingY;
    }

    void PulsarWidget::onChangeEmblem (EmblemId emblemId) {
        _WidgetBase::onChangeEmblem (emblemId);
        emblemWidget->setEmblem (emblemId);
    }

    void PulsarWidget::appendContextMenu (rack::ui::Menu* menu) {
        using rack::ui::Menu;

        _WidgetBase::appendContextMenu (menu);

        // Oversampling options
        menu->addChild (new rack::ui::MenuSeparator);
        menu->addChild (rack::createSubmenuItem ("Oversampling", "", [=] (Menu* menu) {
            auto getOversample = [=] { return static_cast<uint32_t> (moduleT->getParam (PulsarModule::PARAM_OVERSAMPLE)); };
            for (uint32_t accum = 1; accum <= MaxOversample; accum *= 2) {
                auto label = accum > 1 ? fmt::format (FMT_STRING ("{}x"), accum) : "Off";

                menu->addChild (rack::createCheckMenuItem (label, "",
                    [=] { return getOversample () == accum; },
                    [=] {
                        createContextMenuHistory<uint32_t> ("Set Pulsar oversampling factor", [=] (PulsarModule* module, uint32_t value) {
                            APP->engine->setParamValue (module, PulsarModule::PARAM_OVERSAMPLE, value);
                        }, getOversample (), accum);
                    }
                ));
            }
        }));

        // Channel count
        menu->addChild (rack::createSubmenuItem ("Channel count", "", [=] (Menu* menu) {
            auto getCount = [=] { return static_cast<uint32_t> (moduleT->getParam (PulsarModule::PARAM_CHANNEL_COUNT)); };
            for (uint32_t count = 0; count <= Constants::MaxPolyphony; count++) {
                std::string label = "Auto";
                if (count == 1)
                    label = "1 channel";
                else if (count > 1)
                    label = fmt::format (FMT_STRING ("{} channels"), count);

                menu->addChild (rack::createCheckMenuItem (label, "",
                    [=] { return getCount () == count; },
                    [=] {
                        createContextMenuHistory<uint32_t> ("Set Pulsar channel count", [=] (PulsarModule* module, uint32_t value) {
                            APP->engine->setParamValue (module, PulsarModule::PARAM_CHANNEL_COUNT, value);
                        }, getCount (), count);
                    }
                ));
            }
        }));

        // Overlap mode options
        menu->addChild (new rack::ui::MenuSeparator);
        menu->addChild (rack::createMenuLabel ("Overlap mode"));
        menu->addChild (rack::createBoolMenuItem ("Enable", "",
            [=] { return moduleT->getParam (PulsarModule::PARAM_OVERLAP_MODE) > .5f; },
            [=] (bool enable) {
                createContextMenuHistory<bool> ("Set Pulsar overlap mode", [=] (PulsarModule* module, bool enable) {
                    APP->engine->setParamValue (module, PulsarModule::PARAM_OVERLAP_MODE, enable ? 1.f : 0.f);
                }, !enable, enable);
            }
        ));

        auto edgeFactorSlider = new rack::ui::Slider ();
        edgeFactorSlider->quantity = moduleT->getParamQuantity (PulsarModule::PARAM_EDGE_FACTOR);
        edgeFactorSlider->box.size.x = 200.f;
        menu->addChild (edgeFactorSlider);
    }
}