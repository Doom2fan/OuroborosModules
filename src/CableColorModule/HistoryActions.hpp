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

#include "CableColorManager.hpp"

#include "../PluginDef.hpp"

#include <string>

namespace OuroborosModules {
namespace CableColorModule {
    struct ColorManagerHistory : rack::history::Action {
      protected:
        CableColorCollection& getCollection () { return getColorManager ()->colorCollection; }
    };

    struct HistoryReplaceCollection : ColorManagerHistory {
      private:
        CableColorCollection oldCollection;
        CableColorCollection newCollection;

      public:
        HistoryReplaceCollection (const CableColorCollection& oldCollection, const CableColorCollection& newCollection) {
            name = "replace collection";

            this->oldCollection = oldCollection;
            this->newCollection = newCollection;
        }

        void undo () override { getColorManager ()->changeCollection (oldCollection, false); }
        void redo () override { getColorManager ()->changeCollection (newCollection, false); }
    };

    struct AddNewColorHistory : ColorManagerHistory {
      private:
        int32_t index;
        CableColor color;

      public:
        AddNewColorHistory (int32_t index, const CableColor& color) {
            name = "add new cable color";

            this->index = index;
            this->color = color;
        }

        void undo () override { getCollection ().removeColor (index); }
        void redo () override { getCollection ().addColor (index, color); }
    };

    struct DeleteColorHistory : ColorManagerHistory {
      private:
        int32_t index;
        CableColor color;

      public:
        DeleteColorHistory (int32_t index, const CableColor& color) {
            name = "delete cable color";

            this->index = index;
            this->color = color;
        }

        void undo () override { getCollection ().addColor (index, color); }
        void redo () override { getCollection ().removeColor (index); }
    };

    struct ChangeColorHistory : ColorManagerHistory {
      private:
        int32_t index;
        CableColor oldColor;
        CableColor newColor;

      public:
        ChangeColorHistory (
            const std::string& name,
            int32_t index,
            const CableColor& oldColor,
            const CableColor& newColor
        ) {
            this->name = name;

            this->index = index;
            this->oldColor = oldColor;
            this->newColor = newColor;
        }

        void undo () override { getCollection ().setColor (index, oldColor); }
        void redo () override { getCollection ().setColor (index, newColor); }
    };
}
}