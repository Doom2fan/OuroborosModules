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

#include "Automata.hpp"

#include <fmt/format.h>

namespace OuroborosModules::Modules::Automata {
    EditGrid copyBoardToEditGrid (const AutomataBoard& board, AutomataCell editMask) {
        auto boardCopy = board;

        auto editGrid = EditGrid ();
        editGrid.clear ();
        for (int y = 0; y < BoardHeight; y++) {
            for (int x = 0; x < BoardWidth; x++)
                editGrid.at (x, y) = testCellFlag (boardCopy.at (x, y), editMask);
        }

        return editGrid;
    }

    void copyEditGridToBoard (const EditGrid& editGrid, AutomataBoard& board, AutomataCell editMask) {
        for (int y = 0; y < BoardHeight; y++) {
            for (int x = 0; x < BoardWidth; x++) {
                auto& cell = board.at (x, y);
                cell = (cell & ~editMask) | (editGrid.at (x, y) ? editMask : AutomataCell::None);
            }
        }
    }

    void copyEditGridMaskToBoard (
        const EditGrid& oldGrid, const EditGrid& gridMask,
        AutomataBoard& board, AutomataCell editMask
    ) {
        for (int y = 0; y < BoardHeight; y++) {
            for (int x = 0; x < BoardWidth; x++) {
                if (!gridMask.at (x, y))
                    continue;

                auto& cell = board.at (x, y);
                cell = (cell & ~editMask) | (oldGrid.at (x, y) ? editMask : AutomataCell::None);
            }
        }
    }

    void setBoardFromEditMask (const EditGrid& gridMask, bool set, AutomataBoard& board, AutomataCell editMask) {
        for (int y = 0; y < BoardHeight; y++) {
            for (int x = 0; x < BoardWidth; x++) {
                if (!gridMask.at (x, y))
                    continue;

                auto& cell = board.at (x, y);
                cell = set ? (cell | editMask) : (cell & ~editMask);
            }
        }
    }

    /*
     * EditCommand_Clear
     */
    EditCommand_Clear::EditCommand_Clear (const AutomataBoard& board, AutomataCell editMask) {
        switch (editMask) {
            case AutomataCell::FLAG_SeedSet: description = "clear automata seed"; break;
            case AutomataCell::TRIGGER_1: case AutomataCell::TRIGGER_2:
            case AutomataCell::TRIGGER_3: case AutomataCell::TRIGGER_4:
            case AutomataCell::TRIGGER_5: case AutomataCell::TRIGGER_6:
            case AutomataCell::TRIGGER_7: case AutomataCell::TRIGGER_8:{
                description = fmt::format (FMT_STRING ("clear automata trigger set #{}"), triggerFromCell (editMask));
                break;
            }

            default:
                assert (false && "Invalid edit mask.");
                description = "ERROR";
                return;
        }

        oldGrid = copyBoardToEditGrid (board, editMask);
        this->editMask = editMask;
    }

    void EditCommand_Clear::undo (AutomataBoard& board) {
        copyEditGridToBoard (oldGrid, board, editMask);
    }

    void EditCommand_Clear::execute (AutomataBoard& board) {
        for (auto& cell : board)
            cell &= ~editMask;
    }

    /*
     * EditCommand_Toggle
     */
    EditCommand_Toggle::EditCommand_Toggle (
        const AutomataBoard& board, AutomataCell editMask,
        const EditGrid& gridMask, bool set
    ) {
        switch (editMask) {
            case AutomataCell::FLAG_SeedSet: description = "modify automata seed"; break;
            case AutomataCell::TRIGGER_1: case AutomataCell::TRIGGER_2:
            case AutomataCell::TRIGGER_3: case AutomataCell::TRIGGER_4:
            case AutomataCell::TRIGGER_5: case AutomataCell::TRIGGER_6:
            case AutomataCell::TRIGGER_7: case AutomataCell::TRIGGER_8:{
                description = fmt::format (FMT_STRING ("modify automata trigger set #{}"), triggerFromCell (editMask));
                break;
            }

            default:
                assert (false && "Invalid edit mask.");
                description = "ERROR";
                return;
        }

        oldGrid = copyBoardToEditGrid (board, editMask);

        this->editMask = editMask;
        this->gridMask = gridMask;
        editSet = set;
    }

    void EditCommand_Toggle::undo (AutomataBoard& board) {
        copyEditGridMaskToBoard (oldGrid, gridMask, board, editMask);
    }

    void EditCommand_Toggle::execute (AutomataBoard& board) {
        setBoardFromEditMask (gridMask, editSet, board, editMask);
    }

    /*
     * HistoryAutomataEditCommand
     */
    HistoryAutomataEditCommand::HistoryAutomataEditCommand (AutomataModule* module, std::shared_ptr<EditCommand> newCommand) {
        moduleId = module->id;
        command = newCommand;
        name = command->getDescription ();
    }

    void HistoryAutomataEditCommand::undo () {
        auto module = dynamic_cast<AutomataModule*> (APP->engine->getModule (moduleId));
        if (module == nullptr)
            return;

        module->enqueueCommand (command, true);
    }

    void HistoryAutomataEditCommand::redo () {
        auto module = dynamic_cast<AutomataModule*> (APP->engine->getModule (moduleId));
        if (module == nullptr)
            return;

        module->enqueueCommand (command, false);
    }
}