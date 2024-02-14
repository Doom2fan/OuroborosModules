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
#include "Utils.hpp"
#include <osdialog.h>

char* selectSoundFile () {
    static const char FILE_FILTERS [] = "Wave (.wav):wav,WAV;All files (*.*):*.*";
    auto filters = osdialog_filters_parse (FILE_FILTERS);
    DEFER ({ osdialog_filters_free (filters); });

    return osdialog_file (OSDIALOG_OPEN, nullptr, nullptr, filters);
}

ThemeKind getCurrentTheme () {
    return !rack::settings::preferDarkPanels
        ? pluginSettings.global_ThemeLight
        : pluginSettings.global_ThemeDark;
}