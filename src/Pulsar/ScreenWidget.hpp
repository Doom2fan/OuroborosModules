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

#pragma once

#include "../PluginDef.hpp"

#include "Pulsar.hpp"

namespace OuroborosModules::Modules::Pulsar {
    struct ScopePoint {
        float min = std::numeric_limits<float>::infinity ();
        float max = -std::numeric_limits<float>::infinity ();
    };

    struct ScreenWidget : rack_themer::SvgHelper<rack_themer::ThemedWidgetBase<rack::widget::Widget>> {
        struct Internal;

      private:
        PulsarWidget* panelWidget;

        // Widgets
        Widgets::BufferedDrawFunctionWidgetOnLayer* screenFb = nullptr;

        // Screen boxes
        rack::Rect wavePanelBox;
        rack::Rect windowPanelBox;
        rack::Rect infoPanelBox;
        rack::Rect frequencyPanelBox;

        // State
        RGBColor lastDisplayColor = { };

        uint32_t curChannel = 0;
        float curEmissionFrequency = 0.f;
        PulsarParameters curParams = { };

        PulsarFrequencyMode curFrequencyMode = PulsarFrequencyMode::Audio;
        PulsarFormantMode curFormantMode = PulsarFormantMode::AudioRate;
        PulsarMaskingMode curMaskingMode = PulsarMaskingMode::Burst;

        FormantFreqDisplayMode curFormantFreqDisplayMode = FormantFreqDisplayMode::Auto;

        std::vector<ScopePoint> wavePoints = { };
        std::vector<ScopePoint> windowPoints = { };

        void checkDirty ();

        void generateGraph ();

        void renderGraph (NVGcontext* vg, rack::Rect bounds, const std::vector<ScopePoint>& points);
        void renderInfoPanel (NVGcontext* vg, rack::Rect bounds);
        void renderFrequencyPanel (NVGcontext* vg, rack::Rect bounds);
        void renderScreen (NVGcontext* vg);

      public:
        ScreenWidget (PulsarWidget* panelWidget);

        void step () override;
    };

    static const PulsarParameters createBrowserParameters () {
        PulsarParameters params { };

        params.frequency = rack::simd::pow (2, 3.f / 12.f) * rack::dsp::FREQ_C4;
        params.cluster = 4.f;

        params.shapeIndex = 3.f;
        params.shaperAmount = 0.f;

        params.windowIndex = 0.f;
        params.windowSkew = .5f;
        params.windowAmount = 1.f;

        return params;
    }
    static const PulsarParameters browserParameters = createBrowserParameters ();
    static const float browserEmissionFrequency = rack::dsp::FREQ_C4;
}