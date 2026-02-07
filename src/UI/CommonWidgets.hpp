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

#pragma once

#include "../PluginDef.hpp"
#include "ThemeUtils.hpp"
#include "ImageWidget.hpp"

#include <rack_themer.hpp>

namespace OuroborosModules::Widgets {
    struct ScrewWidget : rack_themer::widgets::SvgScrew {
        ScrewWidget () {
            setSvg (Theme::getThemedSvg ("components/Screw", nullptr));
        }
    };

    struct CableJackInput : rack_themer::widgets::SvgPort {
        CableJackInput () { setSvg (Theme::getThemedSvg ("components/CableJack_In", nullptr)); }
    };

    struct CableJackOutput : rack_themer::widgets::SvgPort {
        CableJackOutput () { setSvg (Theme::getThemedSvg ("components/CableJack_Out", nullptr)); }
    };

    struct EmblemWidget : rack_themer::ThemedWidgetBase<rack::widget::TransparentWidget> {
      private:
        rack::math::Vec posCentered;
        float size;

        ImageWidget* imageWidget;
        rack::widget::FramebufferWidget* framebuffer;

        EmblemWidget (const EmblemWidget& x) = delete;
        void operator= (const EmblemWidget& x) = delete;

      public:
        EmblemWidget (
            EmblemId emblemId,
            rack::math::Vec newPos,
            float newSize = Constants::StdEmblemSize
        ) : EmblemWidget (newPos, newSize) {
            setEmblem (emblemId);
        }
        EmblemWidget (rack::math::Vec newPos, float newSize = Constants::StdEmblemSize);

        void update ();

        rack::math::Vec getEmblemPos () { return posCentered; }
        void setEmblemPos (rack::math::Vec newPos);

        float getEmblemSize () { return size; }
        void setEmblemSize (float newSize);

        void setEmblem (EmblemId emblemId) { setEmblem (emblemId, size); }
        void setEmblem (EmblemId emblemId, float newSize);
    };

    template<typename TBase = rack::app::ModuleLightWidget>
    struct ResizableLight : TBase {
      private:
        float curSize;

      public:
        rack::widget::FramebufferWidget* framebuffer;
        rack::widget::TransformWidget* transformWidget;
        rack::widget::SvgWidget* svgWidget;

        ResizableLight () {
            framebuffer = new rack::widget::FramebufferWidget;
            this->addChild (framebuffer);

            transformWidget = new rack::widget::TransformWidget;
            framebuffer->addChild (transformWidget);

            svgWidget = new rack::widget::SvgWidget;
            transformWidget->addChild (svgWidget);

            setSize (1);
        }

        void setSvg (std::shared_ptr<rack::window::Svg> svg) { svgWidget->setSvg (svg); setSize (curSize); }
        void setSvg (std::shared_ptr<rack::window::Svg> svg, float newSize) { svgWidget->setSvg (svg); setSize (newSize); }
        void setSize (float newSize) {
            auto posCenter = this->box.pos.plus (this->box.size.div (2));

            auto svgSize = svgWidget->box.size;
            auto vecAspectRatio = svgSize.div (std::max (svgSize.x, svgSize.y));
            auto realSize = vecAspectRatio.mult (newSize);

            transformWidget->box.size = realSize;
            framebuffer->box.size = realSize;

            transformWidget->identity ();
            transformWidget->scale (realSize.div (svgSize));

            this->box.size = realSize;
            this->box.pos = posCenter.minus (realSize.div (2));

            framebuffer->setDirty ();

            curSize = newSize;
        }
    };

    template<typename TBase = rack::app::ModuleLightWidget>
    struct ResizableVCVLight : ResizableLight<TBase> {
        ResizableVCVLight (float size) {
            this->setSvg (rack::window::Svg::load (rack::asset::system ("res/ComponentLibrary/SmallLight.svg")), size);
        }
    };

    struct SlideSwitch2 : rack_themer::widgets::SvgSwitch {
        SlideSwitch2 () {
            shadow->opacity = 0.0;
            addFrame (Theme::getSvg ("components/Slide2_0"));
            addFrame (Theme::getSvg ("components/Slide2_1"));
        }
    };

    struct SlideSwitch2Inverse : rack_themer::widgets::SvgSwitch {
        SlideSwitch2Inverse () {
            shadow->opacity = 0.0;
            addFrame (Theme::getSvg ("components/Slide2_1"));
            addFrame (Theme::getSvg ("components/Slide2_0"));
        }
    };

    struct MetalKnobSmall : rack_themer::widgets::SvgKnob {
        rack_themer::widgets::SvgWidget* background;

        MetalKnobSmall () {
            minAngle = -0.83 * M_PI;
            maxAngle = 0.83 * M_PI;

            background = new rack_themer::widgets::SvgWidget;
            framebuffer->addChildBelow (background, transformWidget);

            setSvg (Theme::getSvg ("components/KnobMetalSmall"));
            background->setSvg (Theme::getSvg ("components/KnobMetalSmall_BG"));
        }

        MetalKnobSmall (const MetalKnobSmall& x) = delete;
        void operator= (const MetalKnobSmall& x) = delete;
    };

    template<typename TLightBase = rack::componentlibrary::WhiteLight>
    struct LightButton : rack_themer::widgets::SvgSwitch {
        struct ButtonLight : TLightBase {
            ButtonLight () {
                this->borderColor = rack::color::BLACK_TRANSPARENT;
                this->bgColor = rack::color::BLACK_TRANSPARENT;
                this->box.size = rack::math::Vec (21);
            }
        };

        rack::app::ModuleLightWidget* light;

        LightButton () {
            momentary = true;
            addFrame (Theme::getSvg ("components/LightButton"));

            light = new ButtonLight;
            // Move center of light to center of box
            light->box.pos = box.size.div (2).minus (light->box.size.div (2));
            addChild (light);
        }

        rack::app::ModuleLightWidget* getLight () { return light; }

        LightButton (const LightButton& x) = delete;
        void operator= (const LightButton& x) = delete;
    };

    template<typename TLightBase = rack::componentlibrary::WhiteLight>
    struct LightButtonSquare : rack_themer::widgets::SvgSwitch {
        struct ButtonLight : rack::componentlibrary::RectangleLight<TLightBase> {
            ButtonLight () {
                this->borderColor = rack::color::BLACK_TRANSPARENT;
                this->bgColor = rack::color::BLACK_TRANSPARENT;
                this->box.size = rack::math::Vec (21);
            }
        };

        rack::app::ModuleLightWidget* light;

        LightButtonSquare () {
            momentary = true;
            addFrame (Theme::getSvg ("components/LightButtonSquare"));

            light = new ButtonLight;
            // Move center of light to center of box
            light->box.pos = box.size.div (2).minus (light->box.size.div (2));
            addChild (light);

            this->shadow->hide ();
        }

        rack::app::ModuleLightWidget* getLight () { return light; }

        LightButtonSquare (const LightButtonSquare& x) = delete;
        void operator= (const LightButtonSquare& x) = delete;
    };

    struct TrimmerKnob : rack_themer::widgets::SvgKnob {
        rack_themer::widgets::SvgWidget* background;

        TrimmerKnob () {
            minAngle = -0.75 * M_PI;
            maxAngle = 0.75 * M_PI;

            background = new rack_themer::widgets::SvgWidget;
            framebuffer->addChildBelow (background, transformWidget);

            setSvg (Theme::getSvg ("components/Trimmer"));
            background->setSvg (Theme::getSvg ("components/Trimmer_BG"));
        }

        TrimmerKnob (const TrimmerKnob& x) = delete;
        void operator= (const TrimmerKnob& x) = delete;
    };

    template<typename TBase = rack::componentlibrary::GrayModuleLightWidget>
    struct TGreenBlueLight : TBase {
        TGreenBlueLight () {
            this->addBaseColor (rack::componentlibrary::SCHEME_GREEN);
            this->addBaseColor (rack::componentlibrary::SCHEME_BLUE);
        }
    };
    using GreenBlueLight = TGreenBlueLight<>;
}