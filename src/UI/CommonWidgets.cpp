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

#include "CommonWidgets.hpp"

namespace OuroborosModules::Widgets {
    EmblemWidget::EmblemWidget (rack::math::Vec newPos, float newSize) : posCentered (newPos) {
        framebuffer = new rack::widget::FramebufferWidget;
        framebuffer->oversample = 2.0;
        addChild (framebuffer);

        imageWidget = rack::createWidget<ImageWidget> (rack::math::Vec ());
        framebuffer->addChild (imageWidget);

        box.size = rack::math::Vec ();
        setEmblemSize (newSize);
    }

    void EmblemWidget::update () {
        if (imageWidget == nullptr)
            return;

        imageWidget->wrap ();
        imageWidget->setZoom (size);

        box.size = imageWidget->box.size;
        box.pos = posCentered.minus (box.size.div (2.f));

        framebuffer->setDirty ();
    }

    void EmblemWidget::setEmblemPos (rack::math::Vec newPos) {
        if (imageWidget == nullptr)
            return;

        posCentered = newPos;
        update ();
    }

    void EmblemWidget::setEmblemSize (float newSize) {
        if (imageWidget == nullptr)
            return;

        size = newSize;
        update ();
    }

    void EmblemWidget::setEmblem (EmblemId emblemId, float newSize) {
        if (imageWidget == nullptr)
            return;

        if (emblemId.isNone ()) {
            this->hide ();
            return;
        } else
            this->show ();

        imageWidget->svg = imageWidget->svg.withSvg (emblemId.getSvgInstance (Theme::getCurrentTheme ()).svg);
        update ();
    }
}