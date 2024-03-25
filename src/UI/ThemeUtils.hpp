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
#include "../Utils.hpp"

#include <rack_themer.hpp>

#include <memory>
#include <string>

namespace OuroborosModules {
namespace Theme {
    ThemeKind getCurrentTheme ();
    std::shared_ptr<rack_themer::RackTheme> getTheme (ThemeKind theme);

    rack_themer::ThemedSvg getThemedSvg (std::string filePath, std::shared_ptr<rack_themer::RackTheme> theme);
    rack_themer::ThemedSvg getThemedSvg (std::string filePath, ThemeKind theme);
    rack_themer::ThemedSvg getEmblem (EmblemKind emblem, ThemeKind theme);
}
}