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

#include "PluginDef.hpp"
#include "Constants.hpp"

std::string getThemeLabel (ThemeKind theme) {
    switch (theme) {
        case ThemeKind::Unknown: return "Unknown";
        case ThemeKind::Light: return "Light";
        case ThemeKind::Dark: return "Dark";
        case ThemeKind::BlackAndGold: return "Black and Gold";

        default: return "[UNDEFINED THEME]";
    }
}

std::string getEmblemLabel (EmblemKind emblem) {
    switch (emblem) {
        case EmblemKind::Unknown: return "Unknown";
        case EmblemKind::None: return "None";
        case EmblemKind::Dragon: return "Dragon";
        case EmblemKind::BleedingEye: return "Bleeding eye";

        default: return "[UNDEFINED EMBLEM]";
    }
}