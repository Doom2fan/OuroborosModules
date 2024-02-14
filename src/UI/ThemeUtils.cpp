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

std::string getThemedSvg (std::string filePath, ThemeKind theme) {
    std::string nameSuffix;
    switch (theme) {
        case ThemeKind::Dark: nameSuffix = "_Dark"; break;
        case ThemeKind::BlackAndGold: nameSuffix = "_BnG"; break;

        default:
        case ThemeKind::Light:
            nameSuffix = "";
            break;
    }

    return fmt::format (FMT_STRING ("res/{:s}{:s}.svg"), filePath, nameSuffix);
}

std::string getEmblem (EmblemKind emblem, ThemeKind theme) {
    std::string iconPath;
    switch (emblem) {
        case EmblemKind::None: return "";
        default:
        case EmblemKind::Dragon: iconPath = "icons/Dragon"; break;
        case EmblemKind::BleedingEye: iconPath = "icons/BleedingEye"; break;
    }

    return getThemedSvg (iconPath, theme);
}