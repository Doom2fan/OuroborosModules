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

#include "Median.hpp"

#include "../UI/CommonWidgets.hpp"
#include "../UI/WidgetUtils.hpp"
#include "../Utils.hpp"

namespace OuroborosModules::Modules::Median {
    MedianWidget::MedianWidget (MedianModule* module) { constructor (module, "panels/Median"); }

    void MedianWidget::initializeWidget () {
        using rack::createInputCentered;
        using rack::createOutputCentered;
        using rack::createParamCentered;
        using rack::createWidget;
        using rack::componentlibrary::RedGreenBlueLight;
        using rack::math::Rect;
        using rack::math::Vec;
        using Widgets::CableJackInput;
        using Widgets::CableJackOutput;
        using Widgets::createLightCentered;
        using Widgets::ImageWidget;
        using Widgets::MetalKnobSmall;
        using Widgets::ResizableVCVLight;
        using Widgets::ScrewWidget;

        addChild (createWidget<ScrewWidget> (Vec ()));
        addChild (createWidget<ScrewWidget> (Vec (box.size.x, RACK_GRID_HEIGHT).minus (Vec (RACK_GRID_WIDTH))));

        emblemWidget = createWidget<ImageWidget> (Vec ());
        addChild (emblemWidget);
        updateEmblem (curTheme, curEmblem);

        forEachMatched ("input_(\\d+)", [&] (std::vector<std::string> captures, Vec pos) {
            auto i = stoi (captures [0]) - 1;
            if (i < 0 || i > 3)
                return LOG_WARN (FMT_STRING ("Median panel has invalid input #{}"), i);
            addInput (createInputCentered<CableJackInput> (pos, module, MedianModule::INPUT_VALUES + i));
        });

        forEachMatched ("param_Scale(\\d+)", [&] (std::vector<std::string> captures, Vec pos) {
            auto i = stoi (captures [0]) - 1;
            if (i < 0 || i > 3)
                return LOG_WARN (FMT_STRING ("Median panel has invalid scale param #{}"), i);
            addChild (createParamCentered<MetalKnobSmall> (pos, module, MedianModule::PARAM_VAL_SCALE + i));
        });

        forEachMatched ("param_Offs(\\d+)", [&] (std::vector<std::string> captures, Vec pos) {
            auto i = stoi (captures [0]) - 1;
            if (i < 0 || i > 3)
                return LOG_WARN (FMT_STRING ("Median panel has invalid offset param #{}"), i);
            addChild (createParamCentered<MetalKnobSmall> (pos, module, MedianModule::PARAM_VAL_OFFSET + i));
        });

        forEachMatched ("output_(Min|Mid|Max)", [&] (std::vector<std::string> captures, Vec pos) {
            int outputId;
            if (captures [0] == "Min")
                outputId = MedianModule::OUTPUT_MIN;
            else if (captures [0] == "Mid")
                outputId = MedianModule::OUTPUT_MID;
            else if (captures [0] == "Max")
                outputId = MedianModule::OUTPUT_MAX;
            else
                return;

            addChild (createOutputCentered<CableJackOutput> (pos, module, outputId));
        });

        forEachMatched ("light_(Min|Mid|Max)Out", [&] (std::vector<std::string> captures, Rect box) {
            int lightId = MedianModule::LIGHT_OUTPUT;
            if (captures [0] == "Min")
                lightId += MedianModule::OUTLIGHT_Min;
            else if (captures [0] == "Mid")
                lightId += MedianModule::OUTLIGHT_Mid;
            else if (captures [0] == "Max")
                lightId += MedianModule::OUTLIGHT_Max;
            else
                return;

            auto pos = box.getCenter ();
            auto lightSize = std::max (box.size.x, box.size.y);
            addChild (createLightCentered<ResizableVCVLight<RedGreenBlueLight>> (pos, module, lightId, lightSize));
        });
    }

    void MedianWidget::updateEmblem (ThemeId themeId, EmblemId emblemId) {
        if (emblemWidget == nullptr)
            return;

        if (emblemId.isNone ()) {
            emblemWidget->hide ();
            return;
        } else
            emblemWidget->show ();

        emblemWidget->setSvg (emblemId.getSvgInstance (themeId));

        auto emblemPos = findNamed ("widgetLogo").value_or (rack::math::Vec ());
        auto emblemSize = rack::window::mm2px (Constants::StdEmblemSize);

        emblemWidget->setZoom (emblemSize);
        emblemWidget->setSize (rack::math::Vec (emblemSize));
        emblemWidget->box.pos = emblemPos.minus (emblemWidget->box.size.div (2));
    }

    void MedianWidget::onChangeTheme (ThemeId themeId) {
        _WidgetBase::onChangeTheme (themeId);
        updateEmblem (themeId, curEmblem);
    }

    void MedianWidget::onChangeEmblem (EmblemId emblemId) {
        _WidgetBase::onChangeEmblem (emblemId);
        updateEmblem (curTheme, emblemId);
    }

    void MedianWidget::appendContextMenu (rack::ui::Menu* menu) {
        using rack::ui::Menu;

        _WidgetBase::appendContextMenu (menu);

        // Pre-muter
        menu->addChild (new rack::ui::MenuSeparator);
        menu->addChild (rack::createSubmenuItem ("Oversampling", "", [=] (Menu* menu) {
            auto curOversample = static_cast<int> (module->params [MedianModule::PARAM_OVERSAMPLE].getValue ());
            for (int accum = 1; accum <= MedianModule::MaxOversample; accum *= 2) {
                auto label = accum > 1 ? fmt::format (FMT_STRING ("{}x"), accum) : "Off";
                auto isCurrent = accum == curOversample;

                menu->addChild (rack::createCheckMenuItem (label, "",
                    [=] { return isCurrent; },
                    [=] { APP->engine->setParamValue (module, MedianModule::PARAM_OVERSAMPLE, accum); }
                ));
            }
        }));
    }
}