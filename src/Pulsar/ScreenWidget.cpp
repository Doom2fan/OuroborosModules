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

#include "ScreenWidget.hpp"

#include "../Math.hpp"
#include "../UI/CommonWidgets.hpp"
#include "../UI/DotFont.hpp"

#include <fmt/format.h>

namespace OuroborosModules::Modules::Pulsar {
    ScreenWidget* createScreenWidget (rack::Vec pos, PulsarWidget* panelWidget) {
        auto ret = new ScreenWidget (panelWidget);

        ret->box.pos = pos;

        return ret;
    }

    std::string getFont () {
        return Fonts::Dot::nic7pin ();
    }

    ScreenWidget::ScreenWidget (PulsarWidget* panelWidget) {
        using rack::math::Vec;

        auto svg = Theme::getSvg ("components/PulsarScreen");
        setSvg (rack_themer::ThemedSvg (svg, nullptr));

        this->panelWidget = panelWidget;
        box.size = svg->getSize ();

        auto displayBG = rack::createWidget<rack::LedDisplay> (Vec ());
        displayBG->box.size = box.size;
        addChild (displayBG);

        screenFb = new Widgets::BufferedDrawFunctionWidgetOnLayer (
            rack::Vec (),
            box.size,
            [this] (auto* vg) { renderScreen (vg); }
        );
        addChild (screenFb);

        wavePanelBox = findNamedBox ("wavePanel", rack::Rect ());
        windowPanelBox = findNamedBox ("windowPanel", rack::Rect ());
        infoPanelBox = findNamedBox ("infoPanel", rack::Rect ());
        frequencyPanelBox = findNamedBox ("frequencyPanel", rack::Rect ());
    }

    void ScreenWidget::checkDirty () {
        auto module = panelWidget->getPulsar ();
        auto isDirty = false;
        auto needRegen = false;

        // Display channel
        auto channel = module != nullptr ? module->displayChannel : 0;
        if (module != nullptr && channel >= module->engine.getChannelCount ())
            channel = 0;
        if (curChannel != channel) {
            isDirty = needRegen = true;
            curChannel = channel;
        }

        // Parameters
        auto& params = module != nullptr ? module->engine.getParams (curChannel) : browserParameters;
        if (
            curParams.cluster != params.cluster ||

            curParams.shapeIndex != params.shapeIndex ||
            curParams.shaperAmount != params.shaperAmount ||

            curParams.windowIndex != params.windowIndex ||
            curParams.windowSkew != params.windowSkew ||
            curParams.windowAmount != params.windowAmount
        ) {
            isDirty = needRegen = true;
            curParams = params;
        }

        if (curParams.frequency != params.frequency) {
            isDirty = true;
            curParams = params;
        }

        auto emissionFrequency = module != nullptr ? module->engine.getEmissionFrequency (curChannel) : browserEmissionFrequency;
        if (curEmissionFrequency != emissionFrequency) {
            isDirty = true;
            curEmissionFrequency = emissionFrequency;
        }

        // Frequency mode
        auto frequencyMode = module != nullptr ? module->frequencyMode : PulsarFrequencyMode::Audio;
        if (curFrequencyMode != frequencyMode) {
            isDirty = true;
            curFrequencyMode = frequencyMode;
        }

        // Formant mode
        auto formantMode = module != nullptr ? module->formantMode : PulsarFormantMode::Coupled;
        if (curFormantMode != formantMode) {
            isDirty = true;
            curFormantMode = formantMode;
        }

        // Masking mode
        auto maskingMode = module != nullptr ? module->maskingMode : PulsarMaskingMode::Burst;
        if (curMaskingMode != maskingMode) {
            isDirty = true;
            curMaskingMode = maskingMode;
        }

        // Display color
        auto displayColor = panelWidget->getDisplayColor ();
        if (lastDisplayColor != displayColor) {
            isDirty = true;
            lastDisplayColor = displayColor;
        }

        // Formant frequency display mode
        auto formantFreqDisplayMode = module != nullptr
                                    ? module->getFormantFreqDisplayMode ()
                                    : FormantFreqDisplayMode::RelativeVOct;
        if (curFormantFreqDisplayMode != formantFreqDisplayMode) {
            assert (formantFreqDisplayMode != FormantFreqDisplayMode::Auto);

            isDirty = true;
            curFormantFreqDisplayMode = formantFreqDisplayMode;
        }

        // Mark things dirty
        if (!isDirty)
            return;

        if (needRegen)
            generateGraph ();

        screenFb->setDirty ();
    }

    void generateScopePoints (float* samples, std::vector<ScopePoint>& points, uint32_t inputRes) {
        auto stepCount = static_cast<uint32_t> (std::ceil (WavetableLength / static_cast<float> (inputRes)));
        for (uint32_t baseX = 0; baseX < WavetableLength; baseX += stepCount) {
            ScopePoint curPoint {};

            auto curStepCount = std::min (WavetableLength - baseX, stepCount);
            for (uint32_t x = 0; x < curStepCount; x++) {
                auto signal = samples [baseX + x];
                curPoint.min = std::min (signal, curPoint.min);
                curPoint.max = std::max (signal, curPoint.max);
            }

            points.emplace_back (curPoint);
        }
    }

    void ScreenWidget::generateGraph () {
        using Interpolator = DSP::WavetableInterpolators::None;

        auto wavetablesPtr = getWavetables ();
        auto& wavetables = *wavetablesPtr;

        // Calculate graph info
        static constexpr auto factor = 2.f;
        auto waveInputRes = static_cast<uint32_t> (wavePanelBox.size.x * factor);
        auto windowInputRes = static_cast<uint32_t> (windowPanelBox.size.x * factor);
        auto dX = 1.f / WavetableLength;

        // Set up arrays
        wavePoints.clear ();
        wavePoints.reserve (waveInputRes);
        windowPoints.clear ();
        windowPoints.reserve (windowInputRes);

        // Create RNG
        rack::random::Xoroshiro128Plus rng (19813519, 5894321);

        // Fetch parameters
        auto params = curParams;

        auto wave0 = std::min (static_cast<uint32_t> (params.shapeIndex), WavesCount - 2);
        auto waveBlend = std::clamp (params.shapeIndex - wave0, 0.f, 1.f);
        auto wave0Sampler = getSampler (&wavetables.waves [wave0    ], params.shaperAmount, 0);
        auto wave1Sampler = getSampler (&wavetables.waves [wave0 + 1], params.shaperAmount, 0);

        auto window0 = std::min (static_cast<uint32_t> (params.windowIndex), WindowsCount - 2);
        auto windowBlend = std::clamp (params.windowIndex - window0, 0.f, 1.f);
        auto window0Sampler = getSampler (&wavetables.windows [window0    ], params.windowSkew, 0);
        auto window1Sampler = getSampler (&wavetables.windows [window0 + 1], params.windowSkew, 0);

        float waveSamples [WavetableLength];
        float windowSamples [WavetableLength];
        for (uint32_t i = 0; i < WavetableLength; i++) {
            auto x = i * dX;

            // State
            auto windowPhase = x;
            auto wavePhase = x * params.cluster;
            wavePhase -= rack::simd::floor (wavePhase);

            // Generate signal and window
            uint32_t sampleIndex; float sampleFrac;
            calcSampleIndex (wavePhase, sampleIndex, sampleFrac);
            auto wave0Signal = wave0Sampler.sampleFrac<Interpolator> (sampleIndex, sampleFrac);
            float wave1Signal;
            if ((wave0 + 1) < WavesCount - 1)
                wave1Signal = wave1Sampler.sampleFrac<Interpolator> (sampleIndex, sampleFrac);
            else
                wave1Signal = (rng () >> 32) * 2.32830629e-10f * 2.f - 1.f;

            calcSampleIndex (windowPhase, sampleIndex, sampleFrac);
            auto window0Signal = window0Sampler.sampleFrac<Interpolator> (sampleIndex, sampleFrac);
            auto window1Signal = window1Sampler.sampleFrac<Interpolator> (sampleIndex, sampleFrac);

            // Load and crossfade signals
            auto signal = Math::lerp (wave0Signal, wave1Signal, waveBlend);
            auto windowSignal = Math::lerp (window0Signal, window1Signal, windowBlend);

            // Apply windowing
            windowSignal = Math::lerp (1.f, windowSignal, params.windowAmount);
            signal *= windowSignal;

            waveSamples [i] = signal * .5f + .5f;
            windowSamples [i] = windowSignal;
        }

        generateScopePoints (waveSamples, wavePoints, waveInputRes);
        generateScopePoints (windowSamples, windowPoints, windowInputRes);
    }

    inline void renderDot (NVGcontext* vg, rack::Vec pos, float size) {
        nvgCircle (vg, VEC_ARGS (pos - size / 2.f), size / 2.f);
    }

    void ScreenWidget::renderGraph (NVGcontext* vg, rack::Rect bounds, const std::vector<ScopePoint>& points) {
        using Vec = rack::Vec;

        auto displayColor = panelWidget->getDisplayColor ();

        auto marginFac = Vec (2, 1);
        auto box = rack::Rect (
            bounds.pos + Vec (marginFac.x, bounds.size.y - marginFac.y),
            (bounds.size - marginFac * 2.f) * Vec (1.f / (points.size () - 1), -1)
        );
        static constexpr auto lineOffset = 1.f;

        nvgSave (vg);

        nvgBeginPath (vg);
        for (uint32_t i = 0; i < points.size (); i++) {
            auto p = box.interpolate (Vec (i, points [i].max));
            p.y -= lineOffset;

            if (i == 0)
                nvgMoveTo (vg, VEC_ARGS (p));
            else
                nvgLineTo (vg, VEC_ARGS (p));
        }
        for (uint32_t i = points.size (); i > 0; i--) {
            auto p = box.interpolate (Vec (i - 1, points [i - 1].min));
            p.y += lineOffset;

            nvgLineTo (vg, VEC_ARGS (p));
        }
        nvgClosePath (vg);

        nvgFillColor (vg, displayColor);
        nvgFill (vg);

        nvgRestore (vg);
    }

    void ScreenWidget::renderInfoPanel (NVGcontext* vg, rack::Rect bounds) {
        nvgSave (vg);

        auto displayColor = panelWidget->getDisplayColor ();
        auto font = UI::DotFont::loadFont (getFont ());

        nvgTranslate (vg, VEC_ARGS (bounds.pos));
        bounds = bounds.zeroPos ();

        // Generate info string
        std::string freqAudioString = "HI";
        std::string freqLFOString   = "LO";

        std::string trigOffString = "OSC";
        std::string trigOnString  = "TRG";

        std::string formantCoupledString = "LINK";
        std::string formantAudioString   = "D-HI";
        std::string formantLFOString     = "D-LO";

        std::string maskBurstString = "BURST";
        std::string maskStochString = "STOCH";

        std::string freqString;
        std::string trigString;
        std::string formantString;
        std::string maskString;

        switch (curFrequencyMode) {
            default:
            case PulsarFrequencyMode::Audio:        freqString = freqAudioString; trigString = trigOffString; break;
            case PulsarFrequencyMode::Triggered:    freqString = freqAudioString; trigString = trigOnString;  break;
            case PulsarFrequencyMode::LFO:          freqString = freqLFOString;   trigString = trigOffString; break;
            case PulsarFrequencyMode::TriggeredLFO: freqString = freqLFOString;   trigString = trigOnString;  break;
        }

        switch (curFormantMode) {
            default:
            case PulsarFormantMode::Coupled:   formantString = formantCoupledString; break;
            case PulsarFormantMode::AudioRate: formantString = formantAudioString; break;
            case PulsarFormantMode::LFO:       formantString = formantLFOString; break;
        }

        switch (curMaskingMode) {
            default:
            case PulsarMaskingMode::Burst:      maskString = maskBurstString; break;
            case PulsarMaskingMode::Stochastic: maskString = maskStochString; break;
        }

        auto infoText = fmt::format (FMT_STRING ("{} / {} / {} / {}"), freqString, trigString, formantString, maskString);

        // Render info string
        nvgFillColor (vg, displayColor);
        nvgBeginPath (vg);

        auto infoStringPos = bounds.getBottomLeft ();
        font->renderText (
            [=] (rack::Vec pos, float size) { renderDot (vg, pos, size); },
            infoText,
            infoStringPos, 8,
            .5f, UI::DotFontAlign::Bottom | UI::DotFontAlign::Left
        );
        nvgFill (vg);

        nvgRestore (vg);
    }

    void ScreenWidget::renderFrequencyPanel (NVGcontext* vg, rack::Rect bounds) {
        static constexpr const char* FrequencyFormat = "{:.2F} Hz";

        auto displayColor = panelWidget->getDisplayColor ();
        auto font = UI::DotFont::loadFont (getFont ());

        std::string formant;

        switch (curFormantFreqDisplayMode) {
            default:
            case FormantFreqDisplayMode::AbsoluteFreq:
                formant = fmt::format (FMT_STRING (FrequencyFormat), curParams.frequency);
                break;

            case FormantFreqDisplayMode::RelativeVOct: {
                auto vOct = frequencyToVOct (curParams.frequency, curEmissionFrequency);
                formant = fmt::format (FMT_STRING ("{:.2F} V"), vOct);
                break;
            }
        }

        std::pair<bool, std::string> lines [] = {
            { false, "Emission:", },
            { true, fmt::format (FMT_STRING (FrequencyFormat), curEmissionFrequency), },
            { false, "Formant:", },
            { true, formant, },
        };

        // Setup
        nvgSave (vg);

        nvgTranslate (vg, VEC_ARGS (bounds.pos));
        bounds = bounds.zeroPos ();

        nvgFillColor (vg, displayColor);
        nvgBeginPath (vg);

        auto curY = 0.f;
        for (std::size_t i = 0; i < std::size (lines); i++) {
            auto alignRight = lines [i].first;
            auto text = lines [i].second;
            auto textPos = rack::Vec (alignRight ? bounds.size.x : 0, curY);

            font->renderText (
                [=] (rack::Vec pos, float size) { renderDot (vg, pos, size); },
                text,
                textPos, 7,
                .5f, UI::DotFontAlign::Top | (alignRight ? UI::DotFontAlign::Right : UI::DotFontAlign::Left)
            );
            curY += 7 + 2.f;
        }
        nvgFill (vg);

        nvgRestore (vg);
    }

    void ScreenWidget::renderScreen (NVGcontext* vg) {
        renderGraph (vg, wavePanelBox, wavePoints);
        renderGraph (vg, windowPanelBox, windowPoints);
        renderInfoPanel (vg, infoPanelBox);
        renderFrequencyPanel (vg, frequencyPanelBox);
    }

    void ScreenWidget::step () {
        _ThemedWidgetBase::step ();

        checkDirty ();

        screenFb->oversample = APP->window->pixelRatio < 2.0 ? 2.0 : 1.0;
    }
}