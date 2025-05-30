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

#include "Meta.hpp"

#include "../UI/CommonWidgets.hpp"

#include <fmt/format.h>

namespace OuroborosModules::Modules::Meta {
    struct PremuterTimeQuantity : rack::Quantity {
        float* timeSrc;
        float maxTime;

        PremuterTimeQuantity (float* timeSrc, float maxTime) {
            this->timeSrc = timeSrc;
            this->maxTime = maxTime;
        }

        void setValue (float value) override {
            *timeSrc = rack::math::clamp (value, getMinValue (), getMaxValue ());
        }

        float getValue () override { return *timeSrc; }
        float getMinValue () override { return 0; }
        float getMaxValue () override { return maxTime; }
        float getDefaultValue () override { return 0.0f; }
        float getDisplayValue () override { return getValue (); }

        std::string getDisplayValueString () override {
            float valTime = getDisplayValue ();
            return fmt::format (FMT_STRING ("{:.2F}"), valTime);
        }

        void setDisplayValue (float displayValue) override { setValue (displayValue); }
        std::string getLabel () override { return "Pre-mute length"; }
        std::string getUnit () override { return " seconds"; }
    };

    MetaWidget::MetaWidget (MetaModule* module) { constructor (module, "panels/Meta"); }

    void MetaWidget::initializeWidget () {
        using rack::createInputCentered;
        using rack::createOutputCentered;
        using rack::createWidget;
        using rack::math::Vec;
        using Widgets::CableJackInput;
        using Widgets::CableJackOutput;
        using Widgets::ImageWidget;
        using Widgets::ScrewWidget;

        addChild (createWidget<ScrewWidget> (Vec ()));
        addChild (createWidget<ScrewWidget> (Vec (box.size.x, RACK_GRID_HEIGHT).minus (Vec (RACK_GRID_WIDTH))));

        emblemWidget = new Widgets::EmblemWidget (curEmblem, findNamed ("widgetLogo", Vec ()), 75);
        addChild (emblemWidget);

        forEachMatched ("input_(\\d+)", [&] (std::vector<std::string> captures, Vec pos) {
            int i = stoi (captures [0]) - 1;
            addInput (createInputCentered<CableJackInput> (pos, module, MetaModule::INPUT_LEFT + i));
        });
        forEachMatched ("output_(\\d+)", [&] (std::vector<std::string> captures, Vec pos) {
            int i = stoi (captures [0]) - 1;
            addOutput (createOutputCentered<CableJackOutput> (pos, module, MetaModule::OUTPUT_LEFT + i));
        });
    }

    void MetaWidget::updateMetaHandler () {
        auto isEnabled = false;

        isEnabled |= pluginSettings.metaSounds_Enable;

        if (isEnabled && metaHandler == nullptr)
            metaHandler = MetaHandler::getHandler ();
        else if (!isEnabled && metaHandler != nullptr)
            metaHandler = nullptr;

        if (metaHandler == nullptr)
            return;

        if (metaHandler->checkCableConnected ())
            module->cables_NewConnected.store (true);
        if (metaHandler->checkCableDisconnected ())
            module->cables_NewDisconnected.store (true);
    }

    void MetaWidget::step () {
        _WidgetBase::step ();

        // Skip if we're in the module browser.
        if (module == nullptr)
            return;

        updateMetaHandler ();
    }

    void MetaWidget::onChangeEmblem (EmblemId emblemId) {
        _WidgetBase::onChangeEmblem (emblemId);
        emblemWidget->setEmblem (emblemId);
    }

    void MetaWidget::appendContextMenu (rack::ui::Menu* menu) {
        _WidgetBase::appendContextMenu (menu);

        // Pre-muter
        menu->addChild (new rack::ui::MenuSeparator);
        menu->addChild (rack::createMenuLabel ("Pre-muter settings"));

        auto premuterTimeSlider = new Widgets::SimpleSlider (new PremuterTimeQuantity (&module->premuter_SelectedTime, 15.f));
        premuterTimeSlider->box.size.x = 200.f;
        menu->addChild (premuterTimeSlider);
    }

    void MetaWidget::createPluginSettingsMenu (rack::ui::Menu* menu) {
        using rack::createMenuItem;
        using rack::createMenuLabel;

        _WidgetBase::createPluginSettingsMenu (menu);

        menu->addChild (new rack::ui::MenuSeparator);

        // Cable
        menu->addChild (createMenuLabel ("Cable settings"));

        // Meta sounds
        menu->addChild (new rack::ui::MenuSeparator);
        menu->addChild (createMenuLabel ("Meta sounds settings"));
        menu->addChild (rack::createBoolPtrMenuItem ("Enabled", "", &pluginSettings.metaSounds_Enable));

        auto metaSoundsVolumeSlider = new Widgets::SimpleSlider (new Widgets::FloatQuantity (
            "Volume",
            &pluginSettings.metaSounds_Volume,
            0.f, 1.f,
            4,
            nullptr
        ));
        metaSoundsVolumeSlider->box.size.x = 200.f;
        menu->addChild (metaSoundsVolumeSlider);

        for (int i = 0; i < MetaModule::METASOUNDS_LENGTH; i++) {
            auto channelIdx = (MetaModule::MetaSounds_Channels) i;
            auto data = metaSounds_GetData (channelIdx);
            if (data == nullptr)
                continue;

            menu->addChild (rack::createSubmenuItem (data->getName (), "", [=] (rack::ui::Menu* menu) {
                // Enable
                menu->addChild (rack::createBoolMenuItem ("Enabled", "",
                    [=] () { return data->isEnabled (); },
                    [=] (bool enable) { data->setEnabled (enable); }
                ));

                // Volume
                auto volumeSlider = new Widgets::SimpleSlider (new Widgets::FloatQuantity (
                    "Volume",
                    data->getVolumePtr (),
                    0.f, 1.f,
                    4,
                    nullptr
                ));
                volumeSlider->box.size.x = 200.f;
                menu->addChild (volumeSlider);

                // Load
                menu->addChild (createMenuItem ("Load sound", "", [&] () {
                    auto path = selectSoundFile ();
                    if (path == nullptr)
                        return;

                    data->tryChangePath (path, true, true);
                }));

                // Restore default
                menu->addChild (createMenuItem ("Restore default sound", "", [&] () {
                    data->tryChangePath (Constants::MetaSound_DefaultMarker, true, true);
                }));
            }));
        }
    }
}