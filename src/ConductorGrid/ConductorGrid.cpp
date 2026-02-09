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

#include "ConductorGrid.hpp"

#include "../JsonUtils.hpp"

#include <fmt/format.h>

#include <string>

namespace OuroborosModules {
    rack::plugin::Model* modelConductorGrid = createModel<Modules::Conductor::ConductorGridWidget> ("ConductorGrid");
}

namespace OuroborosModules::Modules::Conductor {
    ConductorGridModule::ConductorGridModule () {
        config (NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);

        for (int i = 0; i < PadCount; i++)
            configButton (PARAM_PAD_BUTTON + i, fmt::format (FMT_STRING ("Pad {}"), i + 1));

        clockLights = DSP::ClockDivider (128, rack::random::u32 ());
    }

    json_t* ConductorGridModule::dataToJson () {
        auto rootJ = ConductorExpander::dataToJson ();

        json_object_set_new_int (rootJ, "curPage", curPage);

        return rootJ;
    }

    void ConductorGridModule::dataFromJson (json_t* rootJ) {
        ConductorExpander::dataFromJson (rootJ);

        json_object_try_get_int (rootJ, "curPage", curPage);
    }

    void ConductorGridModule::onDataUpdated (const ConductorDataUpdatedEvent& e) {
        if (queuedPattern != e.queuedPattern)
            queuedPadBlinkTimer.reset ();

        patternCount = e.patternCount;
        currentPattern = e.currentPattern;
        queuedPattern = e.queuedPattern;

        pageCount = static_cast<int> (std::ceil (static_cast<float> (patternCount) / PadCount));
        curPage = std::clamp (curPage, 0, pageCount - 1);
    }

    void ConductorGridModule::processActive (const ProcessArgs& args) {
        if (pageDownButtonTrigger.process (params [PARAM_PAGE_DOWN_BUTTON].getValue ())) {
            curPage = std::max (curPage - 1, 0);
            pageDownLightPulse.trigger (Constants::LightPulseMS);
        }
        if (pageUpButtonTrigger.process (params [PARAM_PAGE_UP_BUTTON].getValue ())) {
            curPage = std::min (curPage + 1, pageCount - 1);
            pageUpLightPulse.trigger (Constants::LightPulseMS);
        }

        for (int i = 0; i < PadCount; i++) {
            if (padButtonTriggers [i].process (params [PARAM_PAD_BUTTON + i].getValue ())) {
                auto selectedPattern = curPage * PadCount + i;
                if (selectedPattern < patternCount) {
                    if (selectedPattern != queuedPattern) {
                        getCoreModule ()->requestEnqueue (selectedPattern);
                        padButtonLightPulses [i].trigger (QueuedPadBlinkInterval / 2.f);
                    } else {
                        getCoreModule ()->requestDequeue ();
                        padButtonLightPulses [i].reset ();
                        padButtonLightPulses [i].trigger (Constants::LightPulseMS);
                    }

                    queuedPadBlinkTimer.reset ();
                }
            }
        }
    }

    void ConductorGridModule::process (const ProcessArgs& args) {
        ConductorExpander::processExpander ();

        enabled = getCoreModule () != nullptr;
        if (enabled)
            processActive (args);

        // Process lights.
        if (clockLights.process ()) {
            auto lightTime = args.sampleTime * clockLights.division;

            lights [LIGHT_PAGE_DOWN_BUTTON].setBrightnessSmooth (boolToLight (pageDownLightPulse.process (lightTime)), lightTime);
            lights [LIGHT_PAGE_UP_BUTTON].setBrightnessSmooth (boolToLight (pageUpLightPulse.process (lightTime)), lightTime);

            if (queuedPadBlinkTimer.process (lightTime) >= QueuedPadBlinkInterval) {
                queuedPadBlinkTimer.reset ();

                auto padIndex = queuedPattern - curPage * PadCount;
                if (enabled && queuedPattern > -1 && padIndex >= 0 && padIndex < PadCount)
                    padButtonLightPulses [padIndex].trigger (QueuedPadBlinkInterval / 2.f);
            }

            for (int i = 0; i < PadCount; i++) {
                auto redLightState = enabled && (curPage * PadCount + i) == currentPattern;
                auto blueLightState = padButtonLightPulses [i].process (lightTime);

                lights [LIGHT_PAD_BUTTON + i * 2].setBrightnessSmooth (boolToLight (redLightState), lightTime);
                lights [LIGHT_PAD_BUTTON + i * 2 + 1].setBrightnessSmooth (boolToLight (blueLightState), lightTime);
            }
        }
    }
}