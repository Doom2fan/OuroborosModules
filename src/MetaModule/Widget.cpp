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

MetaModuleWidget::MetaModuleWidget (MetaModule* module)
    : ModuleWidgetBase<MetaModuleWidget, MetaModule> (module, "panels/MetaModule") {
    using rack::math::Vec;
    using rack::createWidget;
    using rack::createWidgetCentered;
    using rack::createInputCentered;
    using rack::createOutputCentered;
    using rack::componentlibrary::ThemedPJ301MPort;

    addChild (createWidget<ScrewWidget> (Vec (RACK_GRID_WIDTH, 0)));
    addChild (createWidget<ScrewWidget> (Vec (box.size.x - 2 * RACK_GRID_WIDTH, 0)));
    addChild (createWidget<ScrewWidget> (Vec (RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
    addChild (createWidget<ScrewWidget> (Vec (box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

    auto emblemPos = findNamed ("widgetLogo");

    emblemWidget = createWidgetCentered<ImageWidget> (emblemPos.value_or (Vec ()));
    updateEmblem (curTheme, curEmblem);
    emblemWidget->setZoom (75);
    emblemWidget->setSize (Vec (75));
    emblemWidget->box.pos = emblemWidget->box.pos.minus (emblemWidget->box.size.div (2));

    addChild (emblemWidget);

    forEachMatched ("input_(\\d+)", [&](std::vector<std::string> captures, Vec pos) {
        int i = stoi (captures [0]) - 1;
        addInput (createInputCentered<ThemedPJ301MPort> (pos, module, MetaModule::INPUTL_INPUT + i));
    });
    forEachMatched ("output_(\\d+)", [&](std::vector<std::string> captures, Vec pos) {
        int i = stoi (captures [0]) - 1;
        addOutput (createOutputCentered<ThemedPJ301MPort> (pos, module, MetaModule::OUTPUTL_OUTPUT + i));
    });
}

void MetaModuleWidget::updateEmblem (ThemeKind theme, EmblemKind emblem) {
    if (emblemWidget == nullptr)
        return;

    if (emblem == EmblemKind::None) {
        emblemWidget->hide ();
        return;
    } else
        emblemWidget->show ();

    emblemWidget->setSvg (getEmblem (emblem, theme));
}

void MetaModuleWidget::onChangeTheme (ThemeKind kind) {
    ModuleWidgetBase<MetaModuleWidget, MetaModule>::onChangeTheme (kind);
    updateEmblem (kind, curEmblem);
}

void MetaModuleWidget::onChangeEmblem (EmblemKind kind) {
    ModuleWidgetBase<MetaModuleWidget, MetaModule>::onChangeEmblem (kind);
    updateEmblem (curTheme, kind);
}

void MetaModuleWidget::appendContextMenu (rack::ui::Menu* menu) {
    ModuleWidgetBase<MetaModuleWidget, MetaModule>::appendContextMenu (menu);

    // Pre-muter
    menu->addChild (new rack::ui::MenuSeparator);
    menu->addChild (rack::createMenuLabel ("Pre-muter settings"));

    auto premuterTimeSlider = new SimpleSlider (new PremuterTimeQuantity (&module->premuter_SelectedTime, 15.f));
    premuterTimeSlider->box.size.x = 200.f;
    menu->addChild (premuterTimeSlider);
}

void MetaModuleWidget::createPluginSettingsMenu (MetaModuleWidget* widget, rack::ui::Menu* menu) {
    using rack::createMenuItem;
    using rack::createMenuLabel;

    ModuleWidgetBase<MetaModuleWidget, MetaModule>::createPluginSettingsMenu (widget, menu);

    menu->addChild (new rack::ui::MenuSeparator);

    // Cable
    menu->addChild (createMenuLabel ("Cable settings"));

    auto cableCalcRateSlide = new SimpleSlider (new UpdateFrequencyQuantity (
        "Update frequency",
        &pluginSettings.cables_CalcRate,
        15, 250,
        [=] (float v) { module->CalcIntervals (); }
    ));
    cableCalcRateSlide->box.size.x = 200.f;
    menu->addChild (cableCalcRateSlide);

    // Plug sound
    menu->addChild (new rack::ui::MenuSeparator);
    menu->addChild (createMenuLabel ("Plug sound settings"));
    menu->addChild (rack::createBoolMenuItem ("Enabled", "",
        [=] () { return pluginSettings.plugSound_Enable; },
        [=] (bool enable) {
            pluginSettings.plugSound_Enable = enable;
        }
    ));

    auto plugVolumeSlider = new SimpleSlider (new FloatQuantity (
        "Volume",
        &pluginSettings.plugSound_Volume,
        0.f, 1.f,
        4,
        nullptr
    ));
    plugVolumeSlider->box.size.x = 200.f;
    menu->addChild (plugVolumeSlider);

    menu->addChild (createMenuItem ("Load connect sound", "", [=] () {
        auto path = selectSoundFile ();
        if (path == nullptr)
            return;

        pluginSettings.plugSound_ConnectSound = path;
        module->plugSound_RequestLoad = MetaModule::PLUGSOUND_CONNECT;
    }));
    menu->addChild (createMenuItem ("Restore default connect sound", "", [=] () {
        pluginSettings.plugSound_ConnectSound = "<Default>";
        module->plugSound_RequestLoad = MetaModule::PLUGSOUND_CONNECT;
    }));

    menu->addChild (createMenuItem ("Load disconnect sound", "", [=] () {
        auto path = selectSoundFile ();
        if (path == nullptr)
            return;

        pluginSettings.plugSound_DisconnectSound = path;
        module->plugSound_RequestLoad = MetaModule::PLUGSOUND_DISCONNECT;
    }));
    menu->addChild (createMenuItem ("Restore default disconnect sound", "", [=] () {
        pluginSettings.plugSound_DisconnectSound = "<Default>";
        module->plugSound_RequestLoad = MetaModule::PLUGSOUND_DISCONNECT;
    }));
}