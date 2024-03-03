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

struct ImageWidget : rack::widget::TransparentWidget, rack_themer::IThemedWidget {
  private:
    float zoom = 1.f;

  public:
    rack_themer::ThemedSvg svg;
    bool autoSwitchTheme = true;

    ImageWidget () : svg (nullptr, nullptr) { box.size = rack::math::Vec (); }

    float getZoom () { return zoom; }

    /** Sets zoom scale. */
    void setZoom (float zoom);

    /** Sets the box size to the SVG image size. */
    void wrap ();

    /** Sets and wraps the SVG. */
    void setSvg (rack_themer::ThemedSvg svg);

    void draw (const DrawArgs& args) override;

    void onThemeChanged (std::shared_ptr<rack_themer::RackTheme> theme) override;
};