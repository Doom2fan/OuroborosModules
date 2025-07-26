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

#pragma once

#include "../PluginDef.hpp"

#include "AutomataCommon.hpp"

#include <array>

namespace OuroborosModules::Modules::Automata {
    struct AutomataBoard {
      private:
        std::array<AutomataCell, BoardWidth * BoardHeight> boardData;
        bool boardFlip;

      public:
        AutomataBoard ();

        const AutomataCell& at (std::size_t x, std::size_t y) const { return boardData [y * BoardWidth + x]; }
        AutomataCell& at (std::size_t x, std::size_t y) { return boardData [y * BoardWidth + x]; }

        auto begin () { return boardData.begin (); }
        auto end () { return boardData.end (); }
        auto begin () const { return boardData.begin (); }
        auto end () const { return boardData.end (); }

        void flipBoard () { boardFlip = !boardFlip; }
        AutomataCell getLiveFlag     () const { return boardFlip ? AutomataCell::FLAG_LiveB : AutomataCell::FLAG_LiveA; }
        AutomataCell getLiveFlagPrev () const { return boardFlip ? AutomataCell::FLAG_LiveA : AutomataCell::FLAG_LiveB; }

        void clear () { boardData.fill (AutomataCell::None); }

        json_t* dataToJson () const;
        bool dataFromJson (json_t* rootJ);
    };

    struct AutomataLife {
      private:
        bool updated;
        std::atomic<AutomataLifeUpdateIndex> updateIdx = 1;

        // Rule data
        AutomataRules rules;

        // Board data
        AutomataBoard board;

      public:
        AutomataLife ();

        AutomataBoard& getBoard () { return board; }
        const AutomataBoard& getBoard () const { return board; }

        /** Must be called when the board is modified. */
        void markUpdated ();
        bool handleUpdated ();
        AutomataLifeUpdateIndex getUpdateIndex () const { return updateIdx.load (); }
        bool checkUpdated (AutomataLifeUpdateIndex& lastindex);

        void initialize ();
        void reset ();
        void randomize (float density);
        AutomataRules getRules () { return rules; }
        void setRules (AutomataRules newRules) { rules = newRules; }

        void process ();

        json_t* dataToJson () const;
        bool dataFromJson (json_t* rootJ);
    };
}