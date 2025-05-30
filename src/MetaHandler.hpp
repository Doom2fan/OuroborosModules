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

#include "PluginDef.hpp"

#include <memory>

namespace OuroborosModules {
    struct MetaHandler {
        typedef int64_t RackModuleId;
        typedef int64_t TimeUnit;

    private:
        TimeUnit curTime;

        // Cable data
        int cables_prevCount = 0;
        bool cables_hadIncomplete = false;

        bool cables_Connected = false;
        bool cables_Disconnected = false;

        // Module data
        std::unordered_map<RackModuleId, TimeUnit> modules_Time;

        bool modules_Added = false;
        bool modules_Removed = false;
        bool modules_AnyAdded = false;
        bool modules_AnyRemoved = false;

        MetaHandler ();

        void update ();
        void updateCables ();
        void updateModules ();

    public:
        static std::shared_ptr<MetaHandler> getHandler ();

        // Cable data
        bool checkCableConnected () { return cables_Connected; }
        bool checkCableDisconnected () { return cables_Disconnected; }

        // Module data
        bool checkModuleAdded () { return modules_Added; }
        bool checkModuleRemoved () { return modules_Removed; }
    };
}