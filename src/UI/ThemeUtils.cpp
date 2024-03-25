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

#include "ThemeUtils.hpp"

#include <fmt/format.h>

namespace OuroborosModules {
namespace Theme {
    ThemeKind getCurrentTheme () {
        return !rack::settings::preferDarkPanels
            ? pluginSettings.global_ThemeLight
            : pluginSettings.global_ThemeDark;
    }

    std::shared_ptr<rack_themer::RackTheme> getTheme (ThemeKind theme) {
        std::string themeName;
        switch (theme) {
            case ThemeKind::Dark: themeName = "Dark"; break;
            case ThemeKind::BlackAndGold: themeName = "BlackAndGold"; break;

            default:
            case ThemeKind::Light: themeName = "Light"; break;
        }

        auto themePath = fmt::format (FMT_STRING ("res/themes/{:s}.json"), themeName);
        return rack_themer::loadRackTheme (rack::asset::plugin (pluginInstance, themePath));
    }

    rack_themer::ThemedSvg getThemedSvg (std::string filePath, std::shared_ptr<rack_themer::RackTheme> theme) {
        auto svgPath = fmt::format (FMT_STRING ("res/{:s}.svg"), filePath);
        auto svg = rack_themer::loadSvg (rack::asset::plugin (pluginInstance, svgPath));

        return rack_themer::ThemedSvg (svg, theme);
    }

    rack_themer::ThemedSvg getThemedSvg (std::string filePath, ThemeKind themeKind) {
        return getThemedSvg (filePath, getTheme (themeKind));
    }

    rack_themer::ThemedSvg getEmblem (EmblemKind emblem, ThemeKind theme) {
        std::string iconPath;
        switch (emblem) {
            case EmblemKind::None: return rack_themer::ThemedSvg (nullptr, nullptr);
            default:
            case EmblemKind::Dragon: iconPath = "icons/Dragon"; break;
            case EmblemKind::BleedingEye: iconPath = "icons/BleedingEye"; break;
        }

        return getThemedSvg (iconPath, theme);
    }
}
}