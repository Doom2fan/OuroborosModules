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
#include "../Utils.hpp"
#include "ThemeStyles.hpp"

#include <rack_themer.hpp>

#include <memory>
#include <string>

namespace OuroborosModules {
    NVGpaint getColorPaint (NVGcolor color);

namespace Theme {
    ThemeId getCurrentTheme ();

    std::shared_ptr<rack_themer::ThemeableSvg> getSvg (std::string filePath);
    rack_themer::ThemedSvg getThemedSvg (std::string filePath, std::shared_ptr<rack_themer::RackTheme> theme);
    rack_themer::ThemedSvg getThemedSvg (std::string filePath, ThemeId themeId);

    template<const char* T>
    NVGpaint getTextPaint (std::shared_ptr<rack_themer::RackTheme> theme) {
        static rack_themer::KeyedString textStyleName = rack_themer::KeyedString ();

        if (theme == nullptr)
            return NVGpaint ();

        if (!textStyleName.isValid ())
            textStyleName = rack_themer::getKeyedString (T);

        auto textStyle = theme->getClassStyle (textStyleName);
        if (textStyle == nullptr) {
            LOG_WARN (FMT_STRING ("Attempted to get non-existent style \"{}\"."), T);
            return NVGpaint ();
        }
        if (!textStyle->hasFill ()) {
            LOG_WARN (FMT_STRING ("Text style \"{}\" has no fill."), T);
            return NVGpaint ();
        }

        auto textFill = textStyle->getFill ();
        if (!textFill.isColor ()) {
            LOG_WARN (FMT_STRING ("Text style \"{}\" is not a plain color fill."), T);
            return NVGpaint ();
        }

        auto basePaint = NVGpaint ();
        nvgTransformIdentity (basePaint.xform);
        return textFill.getNVGPaint (basePaint);
    }

    template<const char* T>
    std::shared_ptr<rack_themer::Style> getStyle (std::shared_ptr<rack_themer::RackTheme> theme) {
        static rack_themer::KeyedString styleName = rack_themer::KeyedString ();

        if (theme == nullptr)
            return nullptr;

        if (!styleName.isValid ())
            styleName = rack_themer::getKeyedString (T);

        auto style = theme->getClassStyle (styleName);
        if (style == nullptr) {
            LOG_WARN (FMT_STRING ("Attempted to get non-existent style \"{}\"."), T);
            return nullptr;
        }

        return style;
    }
}
}