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

#include "ImageWidget.hpp"

void ImageWidget::setZoom (float zoom) {
    if (zoom == this->zoom)
        return;

    this->zoom = zoom;
}

/** Sets the box size to the SVG image size */
void ImageWidget::wrap () {
    box.size = svg ? svg->getSize () : Vec ();
}

/** Sets and wraps the SVG */
void ImageWidget::setSvg (std::shared_ptr<window::Svg> svg) {
    this->svg = svg;
    wrap ();
}

void ImageWidget::draw (const DrawArgs& args) {
    if (!svg)
        return;

    DrawArgs zoomCtx = args;
    zoomCtx.clipBox.pos = zoomCtx.clipBox.pos.div (zoom);
    zoomCtx.clipBox.size = zoomCtx.clipBox.size.div (zoom);
    // No need to save the state because that is done in the parent
    nvgScale (args.vg, zoom, zoom);

    window::svgDraw (zoomCtx.vg, svg->handle);
}