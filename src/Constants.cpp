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

#include "Constants.hpp"

#include "PluginDef.hpp"
#include "UI/ThemeUtils.hpp"

#include <fmt/format.h>

namespace OuroborosModules {
#define THEMES_BEGIN() StyleCollection themesCollection = StyleCollection ({
#define DEFINE_THEME(key, fileName, displayName) StyleInfo (key, fileName, displayName),
#define THEMES_END() });

#define EMBLEMS_BEGIN() StyleCollection emblemsCollection = StyleCollection ({
#define DEFINE_EMBLEM(key, fileName, displayName) StyleInfo (key, fileName, displayName),
#define EMBLEMS_END() });

#include "Styles_Def.x"

#undef THEMES_BEGIN
#undef DEFINE_THEME
#undef THEMES_END

#undef EMBLEMS_BEGIN
#undef DEFINE_EMBLEM
#undef EMBLEMS_END

    std::optional<StyleInfo> StyleCollection::getStyle (IdType id) const {
        id--;
        if (id < 0 || id >= getMax ())
            return std::nullopt;

        return values [id];
    }

    const StyleCollection& ThemeId::getStyleCollection () { return themesCollection; }

    std::shared_ptr<rack_themer::RackTheme> ThemeId::getThemeInstance () {
        if (auto themeInfo = getStyleInfo ()) {
            auto themePath = fmt::format (FMT_STRING ("res/themes/{:s}.json"), themeInfo->fileName);
            return rack_themer::loadRackTheme (rack::asset::plugin (pluginInstance, themePath));
        }

        return nullptr;
    }

    EmblemId EmblemId::IdNone = EmblemId::getInvalid ();

    const StyleCollection& EmblemId::getStyleCollection () { return emblemsCollection; }

    bool EmblemId::isNone () const {
        if (IdNone.isInvalid ())
            IdNone = getFromKey ("None");

        return *this == IdNone;
    }

    rack_themer::ThemedSvg EmblemId::getSvgInstance (ThemeId themeId) {
        if (auto emblemInfo = getStyleInfo ()) {
            if (emblemInfo->fileName == "??NONE??")
                return rack_themer::ThemedSvg (nullptr, nullptr);

            auto emblemPath = fmt::format (FMT_STRING ("icons/{:s}"), emblemInfo->fileName);
            return Theme::getThemedSvg (emblemPath, themeId);
        }

        return rack_themer::ThemedSvg (nullptr, nullptr);
    }
}