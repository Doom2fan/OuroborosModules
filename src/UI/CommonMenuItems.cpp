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

#include "CommonMenuItems.hpp"

namespace OuroborosModules {
namespace UI {
    void ColorMenuItem::draw (const DrawArgs& args) {
        MenuItem::draw (args);

        // Color circle
        nvgBeginPath (args.vg);
        auto circleRadius = 6.f;
        nvgCircle (args.vg, 8. + circleRadius, box.size.y / 2, circleRadius);
        nvgFillColor (args.vg, color);
        nvgFill (args.vg);
        nvgStrokeWidth (args.vg, 1.);
        nvgStrokeColor (args.vg, rack::color::mult (color, .5));
        nvgStroke (args.vg);
    }

    void SafeMenuItem::onAction (const rack::event::Action& e) {
        auto menu = rack::createMenu ();
        menu->addChild (rack::createMenuLabel (confirmText));
        menu->addChild (rack::createMenuItem (confirmButtonText, "", action, false, alwaysConsume));

        if (alwaysConsume)
            e.consume (this);
    }
}
}