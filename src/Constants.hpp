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

#include <rack_themer.hpp>

#include <memory>
#include <optional>
#include <string>

namespace OuroborosModules {
    struct StyleInfo {
        std::string key;
        std::string fileName;
        std::string displayName;

        StyleInfo () : key (""), fileName (""), displayName ("[UNINITIALIZED STYLE]") {}

        StyleInfo (std::string newKey, std::string newFileName, std::string newDisplayName)
            : key (newKey), fileName (newFileName), displayName (newDisplayName) { }
    };

    class StyleCollection {
      public:
        typedef int IdType;

      private:
        std::vector<StyleInfo> values;
        std::map<std::string, IdType> idMap;

      public:
        StyleCollection (std::vector<StyleInfo> newValues) : values (newValues) {
            for (IdType i = 0; i < (IdType) values.size (); i++)
                idMap [values [i].key] = i + 1;
        }

        IdType getMax () const { return values.size (); }

        std::optional<IdType> getId (std::string key) const {
            if (auto search = idMap.find (key); search != idMap.end ())
                return search->second;

            return -1;
        }

        std::optional<StyleInfo> getStyle (IdType id) const;
    };

    template<typename T>
    struct StyleId {
        typedef int IdType;
        static std::string UnknownKey () { return "??UNKNOWN??"; }

      protected:
        enum {
            ID_INVALID = -1,
            ID_UNKNOWN = 0,
        };

        IdType id;

        T withId (IdType id) {
            auto val = T ();
            val.id = id;
            return val;
        }

      public:
        static T getInvalid () { return T ().withId (ID_INVALID); }
        static T getUnknown () { return T ().withId (ID_UNKNOWN); }
        static T getFromKey (const std::string& key) {
            if (auto id = T::getStyleCollection ().getId (key))
                return T ().withId (*id);
            return getUnknown ();
        }

        static void forEachValue (std::function<void (T)> func) {
            for (int i = 1, max = T::getStyleCollection ().getMax () + 1; i < max; i++)
                func (T ().withId (i));
        }

        bool isInvalid () const { return id <= ID_UNKNOWN || id >= T::getStyleCollection ().getMax (); }
        bool isUnknown () const { return id == ID_UNKNOWN; }

      protected:
        std::optional<StyleInfo> getStyleInfo () const { return T::getStyleCollection ().getStyle (id); }

      public:
        std::string getKey () const { return getStyleInfo ().value_or (T::getUndefinedStyle ()).key; }
        std::string getDisplayName () const { return getStyleInfo ().value_or (T::getUndefinedStyle ()).displayName; }

        json_t* dataToJson () const { return json_string ((!isUnknown () ? getKey () : UnknownKey ()).c_str ()); }

        void dataFromJson (json_t* storageJ) {
            if (!json_is_string (storageJ))
                return;

            std::string key = json_string_value (storageJ);
            id = key != UnknownKey () ? getFromKey (key).id : ID_UNKNOWN;
        }

        bool operator== (const T& rhs) const { return id == rhs.id; }
    };

    struct ThemeId : StyleId<ThemeId> {
      public:
        static std::string getStyleKind () { return "ThemeId"; }
        static const StyleCollection& getStyleCollection ();
        static StyleInfo getUndefinedStyle () { return StyleInfo ("#INVALID THEME KEY#", "", "[UNDEFINED THEME]"); }

        std::shared_ptr<rack_themer::RackTheme> getThemeInstance ();
    };

    struct EmblemId : StyleId<EmblemId> {
      private:
        static EmblemId IdNone;

      public:
        static std::string getStyleKind () { return "EmblemId"; }
        static const StyleCollection& getStyleCollection ();
        static StyleInfo getUndefinedStyle () { return StyleInfo ("#INVALID EMBLEM KEY#", "", "[UNDEFINED EMBLEM]"); }

        bool isNone () const;

        rack_themer::ThemedSvg getSvgInstance (ThemeId themeId);
    };

    namespace Constants {
        static constexpr float StdEmblemSize = 25.745f;

        static constexpr float TriggerThreshLow = .1f;
        static constexpr float TriggerThreshHigh = 2.f;

        static constexpr int MaxPolyphony = rack::engine::PORT_MAX_CHANNELS;

        static const std::string MetaSound_DefaultMarker = "<Default>";
    }

    namespace Colors {
        static const std::map<std::string, NVGcolor> DisplayColors = {
            { "Yellow", nvgRGB (0xFF, 0xD7, 0x14) },
            { "Red", nvgRGB (0xEC, 0x11, 0x2A) },
            { "Purple", nvgRGB (0x8E, 0x14, 0xFF) },
            { "Magenta", nvgRGB (0xFF, 0x14, 0xF1) },
            { "Pink", nvgRGB (0xFF, 0x14, 0x8E) },
            { "Blue", nvgRGB (0x14, 0x51, 0xFF) },
            { "Cyan", nvgRGB (0x14, 0xFC, 0xFF) },
            { "Green", nvgRGB (0x2A, 0xFF, 0x14) },
            { "Orange", nvgRGB (0xFF, 0x99, 0x14) },
        };
    }
}