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

#include "CommonTypes.hpp"

#include "JsonUtils.hpp"
#include "PluginDef.hpp"

namespace OuroborosModules {
    /*
     * RGBColor
     */
    json_t* RGBColor::dataToJson () const {
        auto rootJ = json_object ();

        json_object_set_new_auto (rootJ, "R", r);
        json_object_set_new_auto (rootJ, "G", g);
        json_object_set_new_auto (rootJ, "B", b);
        json_object_set_new_auto (rootJ, "A", a);

        return rootJ;
    }

    bool RGBColor::dataFromJson (json_t* rootJ) {
        if (!json_is_object (rootJ))
            return false;

        auto failed = false;
        failed |= !json_object_try_get_auto (rootJ, "R", r);
        failed |= !json_object_try_get_auto (rootJ, "G", g);
        failed |= !json_object_try_get_auto (rootJ, "B", b);
        failed |= !json_object_try_get_auto (rootJ, "A", a);

        return !failed;
    }

    bool RGBColor::operator== (const RGBColor& rhs) const {
        return r == rhs.r && g == rhs.g && b == rhs.b && a == rhs.a;
    }

    /*
     * SoundSettings
     */
    json_t* SoundSettings::dataToJson () const {
        auto rootJ = json_object ();

        json_object_set_new_auto (rootJ, "Path", path);
        json_object_set_new_auto (rootJ, "Enabled", enabled);
        json_object_set_new_auto (rootJ, "Volume", volume);

        return rootJ;
    }

    bool SoundSettings::dataFromJson (json_t* rootJ) {
        if (!json_is_object (rootJ))
            return false;

        json_object_try_get_auto (rootJ, "Path", path);
        json_object_try_get_auto (rootJ, "Enabled", enabled);
        json_object_try_get_auto (rootJ, "Volume", volume);

        return true;
    }

    /*
     * DisplayColor
     */
    json_t* DisplayColor::dataToJson () const {
        auto rootJ = json_object ();

        json_object_set_new_auto (rootJ, "isDefault", isDefault);
        json_object_set_new_auto (rootJ, "isGlobal", isGlobal);
        json_object_set_new_auto (rootJ, "localColor", localColor);

        return rootJ;
    }

    bool DisplayColor::dataFromJson (json_t* rootJ) {
        if (!json_is_object (rootJ))
            return false;

        auto failed = false;
        failed |= !json_object_try_get_auto (rootJ, "isDefault", isDefault);
        failed |= !json_object_try_get_auto (rootJ, "isGlobal", isGlobal);
        failed |= !json_object_try_get_auto (rootJ, "localColor", localColor);

        return !failed;
    }

    bool DisplayColor::operator== (const DisplayColor& rhs) const {
        return isDefault == rhs.isDefault &&
               isGlobal == rhs.isGlobal &&
               localColor == rhs.localColor;
    }
}