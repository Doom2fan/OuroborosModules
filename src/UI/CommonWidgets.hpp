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

#pragma once

#include "../PluginDef.hpp"
#include "ThemeUtils.hpp"

#include <rack_themer.hpp>

namespace OuroborosModules {
namespace Widgets {
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
}
}