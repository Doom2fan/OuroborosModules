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
#include "ScreenWidget.hpp"

#include <fmt/format.h>

namespace OuroborosModules::Modules::Pulsar {
    PulsarWidget::PulsarWidget (PulsarModule* module) { constructor (module, "panels/Pulsar"); }

    struct ModeButton : Widgets::LightButton<Widgets::RGBLight> {
        ModeButton () {
            momentary = false;
        }
    };

    void PulsarWidget::initializeWidget () {
        using rack::createInputCentered;
        using rack::createLightParamCentered;
        using rack::createOutputCentered;
        using rack::createParamCentered;
        using rack::createWidget;
        using rack::math::Vec;
        using Widgets::CableJackInput;
        using Widgets::CableJackOutput;
        using Widgets::createLightCentered;
        using Widgets::EmblemWidget;
        using Widgets::MetalKnobHuge;
        using Widgets::MetalKnobLarge;
        using Widgets::MetalKnobSmall;
        using Widgets::ScrewWidget;
        using Widgets::TrimmerKnob;

        addChild (createWidget<ScrewWidget> (Vec ()));
        addChild (createWidget<ScrewWidget> (Vec (box.size.x, RACK_GRID_HEIGHT).minus (Vec (RACK_GRID_WIDTH))));

        emblemWidget = new Widgets::EmblemWidget (curEmblem, findNamed ("widgetLogo", Vec ()));
        addChild (emblemWidget);

        /* Params */
        // Frequency
        addChild (createParamCentered<MetalKnobHuge> (findNamed ("param_Freq", Vec ()), moduleT, PulsarModule::PARAM_FREQUENCY));
        addChild (createParamCentered<MetalKnobLarge> (findNamed ("param_Formant", Vec ()), moduleT, PulsarModule::PARAM_FORMANT));
        addChild (createParamCentered<MetalKnobSmall> (findNamed ("param_Cluster", Vec ()), moduleT, PulsarModule::PARAM_CLUSTER));

        // Wave shape
        addChild (createParamCentered<MetalKnobSmall> (findNamed ("param_WaveIndex", Vec ()), moduleT, PulsarModule::PARAM_WAVEINDEX));
        addChild (createParamCentered<MetalKnobSmall> (findNamed ("param_Waveshaper", Vec ()), moduleT, PulsarModule::PARAM_WAVESHAPER));

        // Windowing
        addChild (createParamCentered<MetalKnobSmall> (findNamed ("param_WindowIndex", Vec ()), moduleT, PulsarModule::PARAM_WINDOWINDEX));
        addChild (createParamCentered<MetalKnobSmall> (findNamed ("param_WindowSkew", Vec ()), moduleT, PulsarModule::PARAM_WINDOWSKEW));
        addChild (createParamCentered<MetalKnobSmall> (findNamed ("param_WindowAmount", Vec ()), moduleT, PulsarModule::PARAM_WINDOWAMOUNT));

        // Masking parameters
        addChild (createParamCentered<MetalKnobSmall> (findNamed ("param_BurstCount", Vec ()), moduleT, PulsarModule::PARAM_BURSTCOUNT));
        addChild (createParamCentered<MetalKnobSmall> (findNamed ("param_RestCount", Vec ()), moduleT, PulsarModule::PARAM_RESTCOUNT));

        // Overlap parameters
        addChild (createParamCentered<MetalKnobSmall> (findNamed ("param_EdgeFactor", Vec ()), moduleT, PulsarModule::PARAM_EDGE_FACTOR));

        // Mode switches
        addChild (createLightParamCentered<ModeButton> (findNamed ("param_MaskingMode", Vec ()), moduleT, PulsarModule::PARAM_MASKINGMODE, PulsarModule::LIGHT_MASKINGMODE));
        addChild (createLightParamCentered<ModeButton> (findNamed ("param_FrequencyMode", Vec ()), moduleT, PulsarModule::PARAM_FREQUENCY_MODE, PulsarModule::LIGHT_FREQUENCYMODE));
        addChild (createLightParamCentered<ModeButton> (findNamed ("param_FormantMode", Vec ()), moduleT, PulsarModule::PARAM_FORMANT_MODE, PulsarModule::LIGHT_FORMANTMODE));
        addChild (createLightParamCentered<ModeButton> (findNamed ("param_OverlapMode", Vec ()), moduleT, PulsarModule::PARAM_OVERLAP_MODE, PulsarModule::LIGHT_OVERLAPMODE));

        // CV attenuverters
        addChild (createParamCentered<TrimmerKnob> (findNamed ("param_Formant_CVAtten", Vec ()), moduleT, PulsarModule::PARAM_FORMANT_CV_ATTEN));
        addChild (createParamCentered<TrimmerKnob> (findNamed ("param_Cluster_CVAtten", Vec ()), moduleT, PulsarModule::PARAM_CLUSTER_CV_ATTEN));

        addChild (createParamCentered<TrimmerKnob> (findNamed ("param_WaveIndex_CVAtten", Vec ()), moduleT, PulsarModule::PARAM_WAVEINDEX_CV_ATTEN));
        addChild (createParamCentered<TrimmerKnob> (findNamed ("param_Waveshaper_CVAtten", Vec ()), moduleT, PulsarModule::PARAM_WAVESHAPER_CV_ATTEN));

        addChild (createParamCentered<TrimmerKnob> (findNamed ("param_WindowIndex_CVAtten", Vec ()), moduleT, PulsarModule::PARAM_WINDOWINDEX_CV_ATTEN));
        addChild (createParamCentered<TrimmerKnob> (findNamed ("param_WindowSkew_CVAtten", Vec ()), moduleT, PulsarModule::PARAM_WINDOWSKEW_CV_ATTEN));
        addChild (createParamCentered<TrimmerKnob> (findNamed ("param_WindowAmount_CVAtten", Vec ()), moduleT, PulsarModule::PARAM_WINDOWAMOUNT_CV_ATTEN));

        addChild (createParamCentered<TrimmerKnob> (findNamed ("param_BurstCount_CVAtten", Vec ()), moduleT, PulsarModule::PARAM_BURSTCOUNT_CV_ATTEN));
        addChild (createParamCentered<TrimmerKnob> (findNamed ("param_RestCount_CVAtten", Vec ()), moduleT, PulsarModule::PARAM_RESTCOUNT_CV_ATTEN));

        /* Inputs */
        // Frequency CV
        addInput (createInputCentered<CableJackInput> (findNamed ("input_VOct", Vec ()), moduleT, PulsarModule::INPUT_VOCT));
        addInput (createInputCentered<CableJackInput> (findNamed ("input_FormantCV", Vec ()), moduleT, PulsarModule::INPUT_FORMANT_CV));
        addInput (createInputCentered<CableJackInput> (findNamed ("input_ClusterCV", Vec ()), moduleT, PulsarModule::INPUT_CLUSTER_CV));

        // Wave shape CV
        addInput (createInputCentered<CableJackInput> (findNamed ("input_WaveIndexCV", Vec ()), moduleT, PulsarModule::INPUT_WAVEINDEX_CV));
        addInput (createInputCentered<CableJackInput> (findNamed ("input_WaveshaperCV", Vec ()), moduleT, PulsarModule::INPUT_WAVESHAPER_CV));

        // Windowing CV
        addInput (createInputCentered<CableJackInput> (findNamed ("input_WindowIndexCV", Vec ()), moduleT, PulsarModule::INPUT_WINDOWINDEX_CV));
        addInput (createInputCentered<CableJackInput> (findNamed ("input_WindowSkewCV", Vec ()), moduleT, PulsarModule::INPUT_WINDOWSKEW_CV));
        addInput (createInputCentered<CableJackInput> (findNamed ("input_WindowAmountCV", Vec ()), moduleT, PulsarModule::INPUT_WINDOWAMOUNT_CV));

        // Masking parameters CV
        addInput (createInputCentered<CableJackInput> (findNamed ("input_BurstCountCV", Vec ()), moduleT, PulsarModule::INPUT_BURSTCOUNT_CV));
        addInput (createInputCentered<CableJackInput> (findNamed ("input_RestCountCV", Vec ()), moduleT, PulsarModule::INPUT_RESTCOUNT_CV));

        // Misc
        addInput (createInputCentered<CableJackInput> (findNamed ("input_Sync", Vec ()), moduleT, PulsarModule::INPUT_SYNC));

        /* Outputs */
        addOutput (createOutputCentered<CableJackOutput> (findNamed ("output_Main", Vec ()), moduleT, PulsarModule::OUTPUT_MAIN))
            ->enableConnectToMixmaster ()->enableConnectToNeighbor ();
        addOutput (createOutputCentered<CableJackOutput> (findNamed ("output_Rest", Vec ()), moduleT, PulsarModule::OUTPUT_REST))
            ->enableConnectToMixmaster ()->enableConnectToNeighbor ();

        /* Widgets */
        auto screenBox = findNamedBox ("widgetScreen", rack::math::Rect ());
        screenWidget = createScreenWidget (screenBox.pos, this);
        addChild (screenWidget);
    }

    RGBColor PulsarWidget::getDisplayColor () const {
        return (moduleT != nullptr)
            ? moduleT->displayColor.getColor (&pluginSettings.pulsar_DefaultDisplayColor, &pluginSettings.global_DisplayColor)
            : pluginSettings.pulsar_DefaultDisplayColor.getColor (nullptr, &pluginSettings.global_DisplayColor);
    }

    void PulsarWidget::onChangeEmblem (EmblemId emblemId) {
        _WidgetBase::onChangeEmblem (emblemId);
        emblemWidget->setEmblem (emblemId);
    }

    void PulsarWidget::createLocalStyleMenu (rack::ui::Menu* menu) {
        using rack::ui::Menu;
        using rack::createSubmenuItem;
        using rack::createCheckMenuItem;

        _WidgetBase::createLocalStyleMenu (menu);

        if (moduleT == nullptr)
            return;

        menu->addChild (new rack::ui::MenuSeparator);
        menu->addChild (Widgets::createColorList (
            "Display color",

            &moduleT->displayColor,
            [=] (DisplayColor oldColor, DisplayColor newColor) {
                APP->history->push (new Widgets::HistoryChangeDisplayColor (
                    moduleT, "Automata",
                    [=] (rack::engine::Module* module) {
                        auto moduleT = dynamic_cast<PulsarModule*> (module);
                        return (moduleT != nullptr) ? &moduleT->displayColor : nullptr;
                    },
                    oldColor, newColor
                ));
            },
            &pluginSettings.pulsar_DefaultDisplayColor,
            &pluginSettings.global_DisplayColor
        ));
    }

    void PulsarWidget::createPluginSettingsMenu (rack::ui::Menu* menu) {
        using rack::ui::Menu;
        using rack::createSubmenuItem;
        using rack::createCheckMenuItem;

        _WidgetBase::createPluginSettingsMenu (menu);

        if (moduleT == nullptr)
            return;

        menu->addChild (new rack::ui::MenuSeparator);
        menu->addChild (rack::createMenuLabel ("Visual"));
        menu->addChild (Widgets::createColorList (
            "Default display color",

            &pluginSettings.pulsar_DefaultDisplayColor,
            [=] (DisplayColor oldColor, DisplayColor newColor) { pluginSettings.pulsar_DefaultDisplayColor = newColor; },
            &pluginDefaults.pulsar_DefaultDisplayColor,
            &pluginSettings.global_DisplayColor,
            true
        ));
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

        // Display channel
        menu->addChild (rack::createSubmenuItem ("Display polyphony channel", "", [=] (Menu* menu) {
            for (uint32_t channel = 0; channel < Constants::MaxPolyphony; channel++) {
                auto label = fmt::format (FMT_STRING ("Channel {}"), channel + 1);

                menu->addChild (rack::createCheckMenuItem (label, "",
                    [=] { return moduleT->displayChannel == channel; },
                    [=] {
                        createContextMenuHistory<uint32_t> ("Set Pulsar display poly channel", [=] (PulsarModule* module, uint32_t value) {
                            module->displayChannel = value;
                        }, moduleT->displayChannel, channel);
                    }
                ));
            }
        }));

        // Formant frequency display mode
        menu->addChild (rack::createSubmenuItem ("Formant frequency display mode", "", [=] (Menu* menu) {
            auto addEntry = [=] (FormantFreqDisplayMode newMode, std::string label) {
                menu->addChild (rack::createCheckMenuItem (label, "",
                    [=] { return moduleT->formantFreqDisplayMode == newMode; },
                    [=] {
                        createContextMenuHistory<FormantFreqDisplayMode> ("Set Pulsar formant frequency display mode",
                            [=] (PulsarModule* module, FormantFreqDisplayMode value) {
                                moduleT->formantFreqDisplayMode = value;
                            },
                            moduleT->formantFreqDisplayMode, newMode
                        );
                    }
                ));
            };

            addEntry (FormantFreqDisplayMode::Auto, "Auto");
            addEntry (FormantFreqDisplayMode::AbsoluteFreq, "Absolute Hz");
            addEntry (FormantFreqDisplayMode::RelativeVOct, "Relative V/Oct");
        }));

        // DC blocker options
        menu->addChild (new rack::ui::MenuSeparator);
        menu->addChild (createBoolPtrMenuItemWithHistory (
            "DC blocker (Audio rate)", "",
            "Toggle Pulsar \"DC blocker (Audio rate)\"",
            &PulsarModule::dcBlockerAudio
        ));
        menu->addChild (createBoolPtrMenuItemWithHistory (
            "DC blocker (Low frequency)", "",
            "Toggle Pulsar \"DC blocker (Low frequency)\"",
            &PulsarModule::dcBlockerLFO
        ));
    }
}