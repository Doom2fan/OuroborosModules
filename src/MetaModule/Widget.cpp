/*
 *  OuroborosModules
 *  Copyright (C) 2024 Chronos "phantombeta" Ouroboros
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

#include "MetaModule.hpp"

#include "../UI/CommonWidgets.hpp"

#include <fmt/format.h>

namespace OuroborosModules {
namespace MetaModule {
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

    MetaModuleWidget::MetaModuleWidget (MetaModule* module) { constructor (module, "panels/MetaModule"); }

    void MetaModuleWidget::initializeWidget () {
        using rack::math::Vec;
        using rack::createWidget;
        using rack::createWidgetCentered;
        using rack::createInputCentered;
        using rack::createOutputCentered;
        using Widgets::CableJackWidget;
        using Widgets::ImageWidget;
        using Widgets::ScrewWidget;

        addChild (createWidget<ScrewWidget> (Vec ()));
        addChild (createWidget<ScrewWidget> (Vec (box.size.x, RACK_GRID_HEIGHT).minus (Vec (RACK_GRID_WIDTH))));

        emblemWidget = createWidget<ImageWidget> (Vec ());
        updateEmblem (curTheme, curEmblem);

        addChild (emblemWidget);

        forEachMatched ("input_(\\d+)", [&] (std::vector<std::string> captures, Vec pos) {
            int i = stoi (captures [0]) - 1;
            addInput (createInputCentered<CableJackWidget> (pos, module, MetaModule::INPUTL_INPUT + i));
        });
        forEachMatched ("output_(\\d+)", [&] (std::vector<std::string> captures, Vec pos) {
            int i = stoi (captures [0]) - 1;
            addOutput (createOutputCentered<CableJackWidget> (pos, module, MetaModule::OUTPUTL_OUTPUT + i));
        });

        if (module != nullptr)
            plugSound_CheckChannels ();
    }

    void MetaModuleWidget::updateCableHandler () {
        auto isEnabled = false;

        isEnabled |= pluginSettings.plugSound_Enable;

        if (isEnabled && cables_Handler == nullptr)
            cables_Handler = CableHandler::getHandler ();
        else if (!isEnabled && cables_Handler != nullptr)
            cables_Handler = nullptr;

        if (cables_Handler != nullptr) {
            module->cables_NewConnected.exchange (cables_Handler->checkCableConnected ());
            module->cables_NewDisconnected.exchange (cables_Handler->checkCableDisconnected ());
        }
    }

    void MetaModuleWidget::step () {
        _WidgetBase::step ();

        // Skip if we're in the module browser.
        if (module == nullptr)
            return;

        updateCableHandler ();
        plugSound_CheckChannels ();
    }

    void MetaModuleWidget::updateEmblem (ThemeKind theme, EmblemKind emblem) {
        if (emblemWidget == nullptr)
            return;

        if (emblem == EmblemKind::None) {
            emblemWidget->hide ();
            return;
        } else
            emblemWidget->show ();

        emblemWidget->setSvg (Theme::getEmblem (emblem, theme));

        auto emblemPos = findNamed ("widgetLogo");

        emblemWidget->setZoom (75);
        emblemWidget->setSize (rack::math::Vec (75));
        emblemWidget->box.pos = emblemPos.value_or (rack::math::Vec ()).minus (emblemWidget->box.size.div (2));
    }

    void MetaModuleWidget::onChangeTheme (ThemeKind kind) {
        _WidgetBase::onChangeTheme (kind);
        updateEmblem (kind, curEmblem);
    }

    void MetaModuleWidget::onChangeEmblem (EmblemKind kind) {
        _WidgetBase::onChangeEmblem (kind);
        updateEmblem (curTheme, kind);
    }

    void MetaModuleWidget::appendContextMenu (rack::ui::Menu* menu) {
        _WidgetBase::appendContextMenu (menu);

        // Pre-muter
        menu->addChild (new rack::ui::MenuSeparator);
        menu->addChild (rack::createMenuLabel ("Pre-muter settings"));

        auto premuterTimeSlider = new Widgets::SimpleSlider (new PremuterTimeQuantity (&module->premuter_SelectedTime, 15.f));
        premuterTimeSlider->box.size.x = 200.f;
        menu->addChild (premuterTimeSlider);
    }

    void MetaModuleWidget::createPluginSettingsMenu (rack::ui::Menu* menu) {
        using rack::createMenuItem;
        using rack::createMenuLabel;

        _WidgetBase::createPluginSettingsMenu (menu);

        menu->addChild (new rack::ui::MenuSeparator);

        // Cable
        menu->addChild (createMenuLabel ("Cable settings"));

        // Plug sound
        menu->addChild (new rack::ui::MenuSeparator);
        menu->addChild (createMenuLabel ("Plug sound settings"));
        menu->addChild (rack::createBoolMenuItem ("Enabled", "",
            [] () { return pluginSettings.plugSound_Enable; },
            [] (bool enable) { pluginSettings.plugSound_Enable = enable; }
        ));

        auto plugVolumeSlider = new Widgets::SimpleSlider (new Widgets::FloatQuantity (
            "Volume",
            &pluginSettings.plugSound_Volume,
            0.f, 1.f,
            4,
            nullptr
        ));
        plugVolumeSlider->box.size.x = 200.f;
        menu->addChild (plugVolumeSlider);

        menu->addChild (createMenuItem ("Load connect sound", "", [&] () {
            auto path = selectSoundFile ();
            if (path == nullptr)
                return;

            if (module->plugSound_Channels [MetaModule::PLUGSOUND_CONNECT].load (path, true, true))
                pluginSettings.plugSound_ConnectSound = path;
        }));
        menu->addChild (createMenuItem ("Restore default connect sound", "", [&] () {
            pluginSettings.plugSound_ConnectSound = "<Default>";
        }));

        menu->addChild (createMenuItem ("Load disconnect sound", "", [&] () {
            auto path = selectSoundFile ();
            if (path == nullptr)
                return;

            if (module->plugSound_Channels [MetaModule::PLUGSOUND_DISCONNECT].load (path, true, true))
                pluginSettings.plugSound_DisconnectSound = path;
        }));
        menu->addChild (createMenuItem ("Restore default disconnect sound", "", [&] () {
            pluginSettings.plugSound_DisconnectSound = "<Default>";
        }));
    }
}
}