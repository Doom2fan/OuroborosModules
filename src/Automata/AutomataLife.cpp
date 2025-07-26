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

#include "../JsonUtils.hpp"

namespace OuroborosModules::Modules::Automata {
    AutomataLife::AutomataLife () {
        initialize ();
    }

    void AutomataLife::markUpdated () {
        updated = true;
    }

    bool AutomataLife::handleUpdated () {
        if (updated) {
            ++updateIdx;
            updated = false;
            return true;
        }

        return false;
    }

    bool AutomataLife::checkUpdated (AutomataLifeUpdateIndex& index) {
        auto curIndex = updateIdx.load ();
        auto ret = index < curIndex;

        index = curIndex;
        return ret;
    }

    void AutomataLife::initialize () {
        auto defaultRules = AutomataRules ();
        defaultRules.setBirthFlag (3, true);
        defaultRules.setSurvivalFlag (2, true);
        defaultRules.setSurvivalFlag (3, true);
        setRules (defaultRules);

        board.clear ();
        for (int y = 0; y < BoardHeight; y++) {
            for (int x = 0; x < BoardWidth; x++) {
                if (DefaultBoard_Seed [y] [x] == 1)
                    board.at (x, y) = AutomataCell::FLAG_SeedSet | board.getLiveFlag ();
                else
                    board.at (x, y) = AutomataCell::None;
            }
        }

        markUpdated ();
    }

    void AutomataLife::reset () {
        auto liveFlag = board.getLiveFlag ();

        for (auto& cell : board) {
            cell &= ~AutomataCell::MASK_Live;
            cell |= testCellFlag (cell, AutomataCell::FLAG_SeedSet) ? liveFlag : AutomataCell::None;
        }

        markUpdated ();
    }

    void AutomataLife::randomize (float density) {
        auto liveFlag = board.getLiveFlag ();

        for (auto& cell : board) {
            auto cellLive = rack::random::uniform () < density;
            cell = (cell & ~AutomataCell::MASK_Live) | (cellLive ? liveFlag : AutomataCell::None);
        }

        markUpdated ();
    }

    int checkNeighbor (const AutomataBoard& board, const AutomataCell liveFlag, int x, int y) {
        return testCellFlag (board.at ((x + BoardWidth) % BoardWidth, (y + BoardHeight) % BoardHeight), liveFlag);
    }

    int countNeighbors (const AutomataBoard& board, const AutomataCell liveFlag, int x, int y) {
        return checkNeighbor (board, liveFlag, x + -1, y + -1)
             + checkNeighbor (board, liveFlag, x + -1, y +  0)
             + checkNeighbor (board, liveFlag, x + -1, y +  1)
             + checkNeighbor (board, liveFlag, x +  0, y + -1)
             + checkNeighbor (board, liveFlag, x +  0, y +  1)
             + checkNeighbor (board, liveFlag, x +  1, y + -1)
             + checkNeighbor (board, liveFlag, x +  1, y +  0)
             + checkNeighbor (board, liveFlag, x +  1, y +  1);
    }

    void AutomataLife::process () {
        board.flipBoard ();
        auto liveFlag = board.getLiveFlag ();
        auto liveFlagPrev = board.getLiveFlagPrev ();

        auto birthMask = rules.getBirthMask ();
        auto survivalMask = rules.getSurvivalMask ();
        for (int x = 0; x < BoardWidth; x++) {
            for (int y = 0; y < BoardHeight; y++) {
                auto& curCell = board.at (x, y);
                auto neighbors = countNeighbors (board, liveFlagPrev, x, y);
                auto wasAlive = testCellFlag (curCell, liveFlagPrev);

                auto ruleFormat = 1 << neighbors;
                auto isAlive = ( wasAlive && (ruleFormat & survivalMask) != 0) ||
                               (!wasAlive && (ruleFormat & birthMask   ) != 0);
                curCell = (curCell & ~liveFlag) | (isAlive ? liveFlag : AutomataCell::None);
            }
        }

        markUpdated ();
    }

    json_t* AutomataLife::dataToJson () const {
        auto rootJ = json_object ();

        json_object_set_new_struct (rootJ, "rules", rules);
        json_object_set_new_struct (rootJ, "board", board);

        return rootJ;
    }

    bool AutomataLife::dataFromJson (json_t* rootJ) {
        if (!json_is_object (rootJ))
            return false;

        json_object_try_get_struct (rootJ, "rules", rules);
        json_object_try_get_struct (rootJ, "board", board);

        markUpdated ();

        return true;
    }
}