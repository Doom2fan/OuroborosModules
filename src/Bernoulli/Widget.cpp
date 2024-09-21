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

#include "Bernoulli.hpp"

#include "../UI/CommonWidgets.hpp"
#include "../UI/WidgetUtils.hpp"

#include <fmt/format.h>

namespace OuroborosModules {
namespace Modules {
namespace Bernoulli {
    BernoulliWidget::BernoulliWidget (BernoulliModule* module) { constructor (module, "panels/Bernoulli"); }

    void BernoulliWidget::initializeWidget () {
        using rack::createInputCentered;
        using rack::createOutputCentered;
        using rack::createParamCentered;
        using rack::createWidget;
        using rack::createWidgetCentered;
        using rack::componentlibrary::GreenLight;
        using rack::componentlibrary::RedLight;
        using rack::math::Vec;
        using rack::window::mm2px;
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

        /*
         * Trigger
         */
        forEachMatched ("inputTrigger_(\\d+)", [&] (std::vector<std::string> captures, Vec pos) {
            auto i = stoi (captures [0]) - 1;
            if (i < 0 || i > BernoulliModule::GatesCount)
                return LOG_WARN (FMT_STRING ("Bernoulli panel has invalid trigger input #{}"), i);
            addInput (createInputCentered<CableJackInput> (pos, module, BernoulliModule::INPUT_TRIGGER + i));
        });

        /*
         * Probability
         */
        forEachMatched ("paramProbability_(\\d+)", [&] (std::vector<std::string> captures, Vec pos) {
            auto i = stoi (captures [0]) - 1;
            if (i < 0 || i > BernoulliModule::GatesCount)
                return LOG_WARN (FMT_STRING ("Bernoulli panel has invalid probability param #{}"), i);
            addChild (createParamCentered<MetalKnobSmall> (pos, module, BernoulliModule::PARAM_PROBABILITY + i));
        });
        forEachMatched ("inputProbabilityCV_(\\d+)", [&] (std::vector<std::string> captures, Vec pos) {
            auto i = stoi (captures [0]) - 1;
            if (i < 0 || i > BernoulliModule::GatesCount)
                return LOG_WARN (FMT_STRING ("Bernoulli panel has invalid probability CV input #{}"), i);
            addInput (createInputCentered<CableJackInput> (pos, module, BernoulliModule::INPUT_PROBABILITY_CV + i));
        });
        forEachMatched ("paramProbabilityCVScale_(\\d+)", [&] (std::vector<std::string> captures, Vec pos) {
            auto i = stoi (captures [0]) - 1;
            if (i < 0 || i > BernoulliModule::GatesCount)
                return LOG_WARN (FMT_STRING ("Bernoulli panel has invalid probability CV scale param #{}"), i);
            addChild (createParamCentered<MetalKnobSmall> (pos, module, BernoulliModule::PARAM_PROBABILITY_CV + i));
        });

        /*
         * Mode
         */
        forEachMatched ("paramMode_(\\d+)", [&] (std::vector<std::string> captures, Vec pos) {
            auto i = stoi (captures [0]) - 1;
            if (i < 0 || i > BernoulliModule::GatesCount)
                return LOG_WARN (FMT_STRING ("Bernoulli panel has invalid mode param #{}"), i);
            addChild (createParamCentered<MetalKnobSmall> (pos, module, BernoulliModule::PARAM_MODE + i));
        });

        /*
         * Outputs
         */
        forEachMatched ("output([AB])_(\\d+)", [&] (std::vector<std::string> captures, Vec pos) {
            auto i = stoi (captures [1]) - 1;
            if (i < 0 || i > BernoulliModule::GatesCount)
                return LOG_WARN (FMT_STRING ("Bernoulli panel has invalid output {} #{}"), captures [0], i);

            auto baseIdx = captures [0] == "B" ? BernoulliModule::OUTPUT_B : BernoulliModule::OUTPUT_A;
            addOutput (createOutputCentered<CableJackOutput> (pos, module, baseIdx + i));
        });
        forEachMatched ("lightState([AB])_(\\d+)", [&] (std::vector<std::string> captures, Vec pos) {
            auto i = stoi (captures [1]) - 1;
            if (i < 0 || i > BernoulliModule::GatesCount)
                return LOG_WARN (FMT_STRING ("Bernoulli panel has invalid state light {} #{}"), captures [0], i);

            const float lightSize = mm2px (1.5f);
            if (captures [0] == "A")
                addChild (createLightCentered<ResizableVCVLight<GreenLight>> (pos, module, BernoulliModule::LIGHT_STATE_A + i, lightSize));
            else
                addChild (createLightCentered<ResizableVCVLight<RedLight>> (pos, module, BernoulliModule::LIGHT_STATE_B + i, lightSize));
        });
    }

    void BernoulliWidget::updateEmblem (ThemeId themeId, EmblemId emblemId) {
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

    void BernoulliWidget::onChangeTheme (ThemeId themeId) {
        _WidgetBase::onChangeTheme (themeId);
        updateEmblem (themeId, curEmblem);
    }

    void BernoulliWidget::onChangeEmblem (EmblemId emblemId) {
        _WidgetBase::onChangeEmblem (emblemId);
        updateEmblem (curTheme, emblemId);
    }

    void BernoulliWidget::createLocalStyleMenu (rack::ui::Menu* menu) {
        using rack::ui::Menu;
        using rack::createSubmenuItem;
        using rack::createCheckMenuItem;

        _WidgetBase::createLocalStyleMenu (menu);
    }

    void BernoulliWidget::createPluginSettingsMenu (rack::ui::Menu* menu) {
        using rack::ui::Menu;
        using rack::createSubmenuItem;
        using rack::createCheckMenuItem;

        _WidgetBase::createPluginSettingsMenu (menu);
    }
}
}
}