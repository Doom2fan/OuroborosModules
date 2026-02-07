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

        return rootJ;
    }

    void ConductorGridModule::dataFromJson (json_t* rootJ) {
        ConductorExpander::dataFromJson (rootJ);
    }

    void ConductorGridModule::onDataUpdated (const ConductorDataUpdatedEvent& e) {
        patternCount = e.patternCount;
        currentPattern = e.currentPattern;
        queuedPattern = e.queuedPattern;

        pageCount = static_cast<int> (std::ceil (static_cast<float> (patternCount) / PadCount));
        curPage = std::clamp (curPage, 0, pageCount);
    }

    void ConductorGridModule::queueIndex (int index) {
        if (index != queuedPattern)
            getCoreModule ()->requestEnqueue (index);
        else
            getCoreModule ()->requestDequeue ();
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
                padButtonLightPulses [i].trigger (Constants::LightPulseMS);

                auto selectedPattern = curPage * PadCount + i;
                if (selectedPattern < patternCount)
                    queueIndex (selectedPattern);
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

            for (int i = 0; i < PadCount; i++) {
                auto lightState = padButtonLightPulses [i].process (lightTime);
                lights [LIGHT_PAD_BUTTON + i].setBrightnessSmooth (boolToLight (lightState), lightTime);
            }
        }
    }
}