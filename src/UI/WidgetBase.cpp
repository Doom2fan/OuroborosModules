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

#include "WidgetBase.hpp"
#include "../Utils.hpp"

std::string getLocalThemeLabel (ThemeKind theme) {
    if (theme == ThemeKind::Unknown)
        return "Use default theme";
    return getThemeLabel (theme);
}

std::string getLocalEmblemLabel (EmblemKind emblem) {
    if (emblem == EmblemKind::Unknown)
        return "Use default emblem";
    return getEmblemLabel (emblem);
}