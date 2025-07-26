/*
 *  OuroborosModules
 *  Copyright (C) 2024-2025 Chronos "phantombeta" Ouroboros
 *  Copyright (C) 2016-2023 VCV
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
#include <utility>

namespace OuroborosModules::Modules::Automata {
    struct EditGrid {
        std::array<bool, BoardWidth * BoardHeight> editData = { };

      public:
        EditGrid () { }

        const bool& at (std::size_t x, std::size_t y) const { return editData [y * BoardWidth + x]; }
        bool& at (std::size_t x, std::size_t y) { return editData [y * BoardWidth + x]; }

        auto begin () { return editData.begin (); }
        auto end () { return editData.end (); }
        auto begin () const { return editData.begin (); }
        auto end () const { return editData.end (); }

        void clear () { editData.fill (false); }
    };

    struct EditCommand {
      public:
        bool executed = false;

        virtual ~EditCommand () = default;

        virtual void undo (AutomataBoard& board) = 0;
        virtual void execute (AutomataBoard& board) = 0;
        virtual std::string getDescription () const = 0;
    };

    struct EditCommand_Clear : EditCommand {
      private:
        std::string description;
        AutomataCell editMask;
        EditGrid oldGrid;

      public:
        EditCommand_Clear (const AutomataBoard& board, AutomataCell editMask);

        void undo (AutomataBoard& board) override;
        void execute (AutomataBoard& board) override;
        std::string getDescription () const override { return description; }
    };

    struct EditCommand_Toggle : EditCommand {
      private:
        std::string description;

        EditGrid oldGrid;

        EditGrid gridMask;
        AutomataCell editMask;
        bool editSet;

      public:
        EditCommand_Toggle (const AutomataBoard& board, AutomataCell editMask, const EditGrid& gridMask, bool set);

        void undo (AutomataBoard& board) override;
        void execute (AutomataBoard& board) override;
        std::string getDescription () const override { return description; }
    };

    struct CommandQueue {
        static constexpr int Size = 256;
        using TCommand = std::pair<bool, std::shared_ptr<EditCommand>>;

        std::atomic<size_t> start {0};
        std::atomic<size_t> end {0};
        TCommand data [Size];

        void enqueue (std::shared_ptr<EditCommand> cmd, bool undo) {
            size_t i = end % Size;
            data [i] = std::make_pair (undo, cmd);
            end++;
        }

        TCommand consume () {
            size_t i = start % Size;
            auto t = data [i];
            data [i] = std::make_pair (false, nullptr);
            start++;
            return t;
        }

        void clear () {
            while (!empty ())
                consume ();
        }

        bool empty () const { return start >= end; }
    };
}