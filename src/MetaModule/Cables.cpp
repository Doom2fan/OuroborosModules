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

#include "MetaModule.hpp"

void MetaModule::cables_Process (const ProcessArgs& args, bool& cableConnected, bool& cableDisconnected) {
    cableConnected = cableDisconnected = false;

    if (args.frame % cable_CheckInterval != 0)
        return;

    auto cableContainer = APP->scene->rack->getCableContainer ();
    auto incompleteCable = APP->scene->rack->getIncompleteCable ();

    if (cableContainer == nullptr)
        return;

    bool hasIncompleteCable = incompleteCable != nullptr;
    int cableCount = 0;

    for (auto it = cableContainer->children.begin (), it_end = cableContainer->children.end (); it != it_end; ++it) {
        auto cable = dynamic_cast<CableWidget*> (*it);
        if (cable == nullptr || !cable->isComplete ())
            continue;

        cableCount++;
    }

    if (hasIncompleteCable && !cable_HadIncompleteCable) {
        if (cableCount == cable_PrevCableCount)
            cableConnected = true;
        else if (cableCount < cable_PrevCableCount)
            cableDisconnected = true;
    } else if (!hasIncompleteCable && cable_HadIncompleteCable) {
        if (cableCount > cable_PrevCableCount)
            cableConnected = true;
        else
            cableDisconnected = true;
    }

    cable_PrevCableCount = cableCount;
    cable_HadIncompleteCable = hasIncompleteCable;
}