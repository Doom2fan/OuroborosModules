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

#include <string>

namespace OuroborosModules {
    enum class ThemeKind {
        INVALID = -1,

        Unknown = 0,
        FirstTheme = 1,
        Light = FirstTheme,
        Dark,
        BlackAndGold,
        ThemeCount,
    };

    enum class EmblemKind {
        INVALID = -1,

        Unknown = 0,
        FirstEmblem = 1,
        None = FirstEmblem,
        Dragon,
        BleedingEye,
        EmblemCount,
    };

    std::string getThemeLabel (ThemeKind theme);
    std::string getEmblemLabel (EmblemKind emblem);
}