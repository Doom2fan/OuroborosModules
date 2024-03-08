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
#include <rack_themer.hpp>

struct ScrewWidget : rack::widget::Widget {
    rack::widget::FramebufferWidget* framebuffer;
    rack_themer::widgets::SvgWidget* svgWidget;

    ScrewWidget () {
        framebuffer = new rack::widget::FramebufferWidget;
        addChild (framebuffer);

        svgWidget = new rack_themer::widgets::SvgWidget;
        framebuffer->addChild (svgWidget);

        setSvg (getThemedSvg ("components/Screw", getCurrentTheme ()));
    }

    void setSvg (rack_themer::ThemedSvg svg) {
        if (svgWidget->svg == svg)
            return;

        svgWidget->setSvg (svg);
        box.size = framebuffer->box.size = svgWidget->box.size;

        framebuffer->setDirty ();
    }
};

struct CableJackWidget : rack_themer::widgets::SvgPort {
    CableJackWidget () { setSvg (getThemedSvg ("components/CableJack", getCurrentTheme ())); }
};