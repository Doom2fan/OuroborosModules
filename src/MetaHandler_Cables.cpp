/*
 *  OuroborosModules
 *  Copyright (C) 2024-2025 Chronos "phantombeta" Ouroboros
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

#include "MetaHandler.hpp"

#include "Math.hpp"
#include "Utils.hpp"

#include <functional>

namespace OuroborosModules {
    static constexpr float CablePlugDist = 14.f;

    static rack::math::Vec getSlumpPos (rack::math::Vec pos1, rack::math::Vec pos2) {
        auto dist = pos1.minus (pos2).norm ();
        auto avg = pos1.plus (pos2).div (2);

        // Lower average point as distance increases
        avg.y += (1.f - rack::settings::cableTension) * (150.f + 1.f * dist);

        return avg;
    }

    void MetaCableWidget::draw (const DrawArgs& args) { }

    float getCableBrightness (rack::app::CableWidget* cable) {
        if (cable->isComplete ()) {
            auto output = &cable->getCable ()->outputModule->outputs [cable->getCable ()->outputId];

            if (output->isPolyphonic ())
                return output->plugLights [2].getBrightness ();
            else {
                return std::max (
                    output->plugLights [0].getBrightness (),
                    output->plugLights [1].getBrightness ()
                );
            }
        } else
            return 0.f;
    }

    void drawCableLights (const rack::widget::Widget::DrawArgs& args, rack::widget::Widget* cableContainer) {
        static constexpr float radius = 1.f;
        static constexpr float oradius = radius + std::min (radius * 4.f, 15.f);
        static constexpr float LightSpacing = 15.f;

        if (!pluginSettings.metaCables_Lights.lights_Enabled)
            return;

        const float haloBrightness = rack::settings::haloBrightness;
        if (haloBrightness == 0.f)
            return;

        nvgSave (args.vg);

        nvgGlobalTint (args.vg, nvgRGBAf (1, 1, 1, 1));
        nvgGlobalCompositeBlendFunc (args.vg, NVG_ONE_MINUS_DST_COLOR, NVG_ONE);

        for (auto it = cableContainer->children.begin (), it_end = cableContainer->children.end (); it != it_end; ++it) {
            auto cable = dynamic_cast<rack::app::CableWidget*> (*it);
            if (cable == nullptr)
                continue;

            auto lightBrightness = haloBrightness * getCableBrightness (cable);
            if (lightBrightness == 0.f)
                continue;

            nvgSave (args.vg);

            auto outputPos = cable->getOutputPos ();
            auto inputPos = cable->getInputPos ();
            auto slump = getSlumpPos (outputPos, inputPos);
            outputPos = outputPos.plus (slump.minus (outputPos).normalize ().mult (CablePlugDist));
            inputPos = inputPos.plus (slump.minus (inputPos).normalize ().mult (CablePlugDist));

            Math::BezierCurve curve (outputPos, slump, inputPos);
            auto lightCount = std::round (curve.getArcLength () / LightSpacing);
            curve.forEvenSpacing (lightCount, [&] (int n, float t) {
                auto pt = curve.evaluate (t);

                // Light halo
                nvgBeginPath (args.vg);
                nvgRect (args.vg, pt.x - oradius, pt.y - oradius, 2.f * oradius, 2.f * oradius);

                auto icol = rack::color::mult (cable->color, lightBrightness);
                nvgFillPaint (args.vg, nvgRadialGradient (args.vg, pt.x, pt.y, radius, oradius, icol, nvgRGBA (0, 0, 0, 0)));
                nvgFill (args.vg);
            });

            nvgRestore (args.vg);
        }

        nvgRestore (args.vg);
    }

    void drawCable (const rack::widget::Widget::DrawArgs& args, int layer, rack::app::CableWidget* cable) {
        auto cableColor = cable->color;

        // Cable shadow and cable
        auto opacity = rack::settings::cableOpacity;
        auto thick = false;

        if (cable->isComplete ()) {
            auto output = &cable->getCable ()->outputModule->outputs [cable->getCable ()->outputId];
            // Increase thickness if output port is polyphonic
            if (output->isPolyphonic ())
                thick = true;

            // Draw opaque if mouse is hovering over a connected port
            auto hoveredWidget = APP->event->hoveredWidget;
            if (cable->outputPort == hoveredWidget || cable->inputPort == hoveredWidget)
                opacity = 1.f;
            // Draw translucent cable if not active (i.e. 0 channels)
            else if (output->getChannels () == 0)
                opacity *= .5f;
        } else
            opacity = 1.f; // Draw opaque if the cable is incomplete

        if (opacity <= 0.f)
            return;

        nvgAlpha (args.vg, std::pow (opacity, 1.5f));

        auto outputPos = cable->getOutputPos ();
        auto inputPos = cable->getInputPos ();

        float thickness = thick ? 9.f : 6.f;

        // The endpoints are off-center
        auto slump = getSlumpPos (outputPos, inputPos);
        outputPos = outputPos.plus (slump.minus (outputPos).normalize ().mult (CablePlugDist));
        inputPos = inputPos.plus (slump.minus (inputPos).normalize ().mult (CablePlugDist));

        nvgLineCap (args.vg, NVG_ROUND);
        // Avoids glitches when cable is bent
        nvgLineJoin (args.vg, NVG_ROUND);

        if (layer == -1) {
            // Draw cable shadow
            auto shadowSlump = slump.plus (rack::math::Vec (0, 30));
            nvgBeginPath (args.vg);
            nvgMoveTo (args.vg, VEC_ARGS (outputPos));
            nvgQuadTo (args.vg, VEC_ARGS (shadowSlump), VEC_ARGS (inputPos));
            auto shadowColor = nvgRGBAf (0, 0, 0, .10f);
            nvgStrokeColor (args.vg, shadowColor);
            nvgStrokeWidth (args.vg, thickness - 1.f);
            nvgStroke (args.vg);
        } else if (layer == 0) {
            // Draw cable outline
            nvgBeginPath (args.vg);
            nvgMoveTo (args.vg, VEC_ARGS (outputPos));
            nvgQuadTo (args.vg, VEC_ARGS (slump), VEC_ARGS (inputPos));
            nvgStrokeColor (args.vg, rack::color::mult (cableColor, .8f));
            nvgStrokeWidth (args.vg, thickness);
            nvgStroke (args.vg);

            // Draw cable
            nvgStrokeColor (args.vg, rack::color::mult (cableColor, .95f));
            nvgStrokeWidth (args.vg, thickness - 1.f);
            nvgStroke (args.vg);

            // Draw cable light
            if (pluginSettings.metaCables_Lights.glow_Enabled) {
                nvgGlobalCompositeBlendFunc (args.vg, NVG_ONE_MINUS_DST_COLOR, NVG_ONE);
                nvgGlobalTint (args.vg, nvgRGBAf (1, 1, 1, opacity));

                auto cableBrightness = pluginSettings.metaCables_Lights.glow_Intensity;

                if (pluginSettings.metaCables_Lights.glow_FollowSignal)
                    cableBrightness *= getCableBrightness (cable);

                nvgStrokeColor (args.vg, rack::color::mult (cableColor, cableBrightness));
                nvgStrokeWidth (args.vg, thickness);
                nvgStroke (args.vg);
            }
        }
    }

    void drawCables (const rack::widget::Widget::DrawArgs& args, int layer, rack::widget::Widget* cableContainer) {
        nvgSave (args.vg);

        // Iterate the cables.
        for (auto it = cableContainer->children.begin (), it_end = cableContainer->children.end (); it != it_end; ++it) {
            auto cable = dynamic_cast<rack::app::CableWidget*> (*it);
            if (cable == nullptr)
                continue;

            nvgSave (args.vg);
            drawCable (args, layer, cable);
            nvgRestore (args.vg);
        }

        nvgRestore (args.vg);
    }

    void MetaCableWidget::drawLayer (const DrawArgs& args, int layer) {
        if (layer != 3)
            return;

        // Get the cable container.
        auto cableContainer = APP->scene->rack->getCableContainer ();
        if (cableContainer == nullptr)
            return;

        drawCables (args, -1, cableContainer);
        drawCables (args,  0, cableContainer);
        drawCableLights (args, cableContainer);
    }

    void MetaHandler::updateCables () {
        if (APP == nullptr || APP->scene == nullptr || APP->scene->rack == nullptr)
            return;

        // Get the cable container.
        auto cableContainer = APP->scene->rack->getCableContainer ();
        if (cableContainer == nullptr)
            return;

        // Handle the meta cable widget's visibility.
        if (hasMetaModule () && pluginSettings.metaCables_Lights.needHandler ()) {
            if (metaCableWidget == nullptr) {
                metaCableWidget = new MetaCableWidget ();
                APP->scene->rack->addChild (metaCableWidget);
                metaCableWidget->hide ();
            }

            if (!metaCableWidget->isVisible ()) {
                metaCableWidget->show ();
                cableContainer->hide ();
            }
        } else {
            if (metaCableWidget != nullptr && metaCableWidget->isVisible ()) {
                metaCableWidget->hide ();
                cableContainer->show ();
            }
        }

        // Iterate the cables.
        auto incompleteCable = Utils::getIncompleteCable ();
        auto hasIncompleteCable = incompleteCable != nullptr;
        std::size_t cableCount = 0;

        for (auto it = cableContainer->children.begin (), it_end = cableContainer->children.end (); it != it_end; ++it) {
            auto cable = dynamic_cast<rack::app::CableWidget*> (*it);
            if (cable == nullptr || !cable->isComplete ())
                continue;

            cableCount++;
        }

        // Determine if any cable has been connected or disconnected.
        cables_Connected = cables_Disconnected = false;

        if (hasIncompleteCable && !cables_hadIncomplete) {
            if (cableCount == cables_prevCount)
                cables_Connected = true;
            else if (cableCount < cables_prevCount)
                cables_Disconnected = true;
        } else if (!hasIncompleteCable && cables_hadIncomplete) {
            if (cableCount > cables_prevCount)
                cables_Connected = true;
            else
                cables_Disconnected = true;
        } else if (hasIncompleteCable == cables_hadIncomplete && !modules_AnyAdded && !modules_AnyRemoved && curTime > 0) {
            if (cableCount > cables_prevCount)
                cables_Connected = true;
            else if (cableCount < cables_prevCount)
                cables_Disconnected = true;
        }

        cables_prevCount = cableCount;
        cables_hadIncomplete = hasIncompleteCable;
    }
}