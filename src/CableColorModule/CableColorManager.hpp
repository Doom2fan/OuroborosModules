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

#include <rack.hpp>

#include <string>

namespace OuroborosModules {
namespace CableColorModule {
    struct CableColorKey {
        int button = -1;
        int key = -1;
        int mods = 0;

        CableColorKey () { }
        CableColorKey (int button, int key, int mods) {
            this->button = button;
            this->key = key;
            this->mods = mods;
        }

        bool isMapped () const { return button != -1 || key != -1; }
        const std::string keyText () const;

        bool matches (CableColorKey otherKey);

        json_t* dataToJson ();
        bool dataFromJson (json_t* keyJ);
    };

    struct CableColor {
      public:
        NVGcolor color = rack::color::WHITE;
        CableColorKey key;
        std::string label;

        CableColor () : color (), key (), label () { }
        CableColor (NVGcolor newColor, CableColorKey newKey, std::string newLabel)
            : color (newColor), key (newKey), label (newLabel) { }

        json_t* dataToJson ();
        bool dataFromJson (json_t* colorJ);
    };

    struct CableColorCollection {
      private:
        std::string name = "";
        std::vector<CableColor> colors;

      public:
        CableColorCollection () { }
        CableColorCollection (std::string name) : name (name) { }

        uint32_t count () const { return colors.size (); }
        const CableColor& operator [] (uint32_t i) const { return colors [i]; }

        const std::string& getName () const { return name; }
        void setName (std::string newName) { name = newName; }

        void clear () { colors.clear (); }
        void removeColor (uint32_t index);
        void addColor (CableColor color) { colors.push_back (color); }
        void addColor (NVGcolor color, CableColorKey key, std::string label) { addColor (CableColor (color, key, label)); }
        void addColor (uint32_t index, CableColor color);
        void addColor (uint32_t index, NVGcolor color, CableColorKey key, std::string label)
            { addColor (index, CableColor (color, key, label)); }
        void setColor (uint32_t index, CableColor color) { colors [index] = color; }

        void resetToDefaults ();

        json_t* dataToJson ();
        bool dataFromJson (json_t* collectionJ);
    };

    struct CollectionsStorage {
      private:
        std::map<std::string, CableColorCollection> collections;
        std::string defaultCollectionName;

      public:
        static CollectionsStorage defaults ();

        uint32_t count () const { return collections.size (); }
        bool tryGetCollection (const std::string& key, CableColorCollection& collection) const;

        void addCollection (const CableColorCollection& collection) { collections [collection.getName ()] = collection; }
        void removeCollection (const std::string& collectionName);

        auto begin () const { return collections.begin (); }
        auto end () const { return collections.end (); }

        std::string getDefaultCollectionName () { return defaultCollectionName; }
        bool tryGetDefaultCollection (CableColorCollection& collection) const;
        bool setDefaultCollection (const std::string& collectionName);

        json_t* dataToJson ();
        void dataFromJson (json_t* storageJ);
    };

    struct ColorManagerHistory;
    struct CableColorManager {
        friend ColorManagerHistory;

        enum LearnMode {
            Off,
            LearnColor,
            LearnKeyPtr,
        };

      private:
        CableColorCollection colorCollection;
        uint32_t curColorIndex = 0;

        LearnMode learnMode = LearnMode::Off;
        uint32_t learnIndex = 0;
        CableColorKey* learnKeyPtr = nullptr;

        void setNextCableColorId ();

        void unsetLearnMode ();

        void setColorKey (uint32_t index, CableColorKey key);

      public:
        CableColorManager ();

        uint32_t getCurrentColor () { return curColorIndex; }
        void setCurrentColor (uint32_t index, bool forced = false);
        void updateCurrentColor ();
        void replacePatchCableColor (std::vector<rack::app::CableWidget*>& cables, uint32_t index);
        void replacePatchCableColorAll (std::vector<rack::app::CableWidget*>& cables, bool random);

        const CableColorCollection& getCollection () const { return colorCollection; }
        void changeCollection (const CableColorCollection& collection, bool createHistory);
        void clearColors (bool createHistory);

        bool isLearnMode () const { return learnMode != LearnMode::Off; }
        void setLearnMode (std::string keyName, uint32_t colorIndex);
        void setLearnMode (std::string keyName, CableColorKey* key);

        void addNewColor (CableColor color);
        void addNewColor (NVGcolor color, CableColorKey key, std::string label) { addNewColor (CableColor (color, key, label)); }
        void removeColor (uint32_t index);
        void setColor (uint32_t index, NVGcolor color);
        void setColorLabel (uint32_t index, std::string label);
        void unsetColorKey (uint32_t index) { return setColorKey (index, CableColorKey ()); }

        bool handleKey (CableColorKey key);
        bool learnKey (CableColorKey key);

        json_t* dataToJson ();
        bool dataFromJson (json_t* managerJ);
    };

    std::shared_ptr<CableColorManager> getColorManager ();
}
}