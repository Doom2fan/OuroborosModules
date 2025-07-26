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
    AutomataBoard::AutomataBoard () {
        boardFlip = false;
        clear ();
    }

    json_t* AutomataBoard::dataToJson () const {
        auto rootJ = json_object ();

        json_object_set_new_bool (rootJ, "boardFlip", boardFlip);

        auto dataJ = json_array ();
        for (int y = 0; y < BoardHeight; y++) {
            auto rowJ = json_array ();
            for (int x = 0; x < BoardWidth; x++)
                json_array_append_new (rowJ, json_integer (static_cast<AutomataCellBase> (at (x, y))));

            json_array_append_new (dataJ, rowJ);
        }
        json_object_set_new (rootJ, "dataArray", dataJ);

        return rootJ;
    }

    bool AutomataBoard::dataFromJson (json_t* rootJ) {
        if (!json_is_object (rootJ))
            return false;

        json_object_try_get_bool (rootJ, "boardFlip", boardFlip);

        auto dataJ = json_object_get (rootJ, "dataArray");
        if (!json_is_array (dataJ) || json_array_size (dataJ) != BoardHeight)
            return false;

        for (int y = 0; y < BoardHeight; y++) {
            auto rowJ = json_array_get (dataJ, y);
            if (json_array_size (rowJ) != BoardWidth)
                return false;

            for (int x = 0; x < BoardWidth; x++) {
                auto cellJ = json_array_get (rowJ, x);
                if (!json_is_integer (cellJ))
                    return false;

                at (x, y) = static_cast<AutomataCell> (json_integer_value (cellJ));
            }
        }

        return rootJ;
    }
}