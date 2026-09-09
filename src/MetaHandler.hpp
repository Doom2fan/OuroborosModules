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
    struct MetaCableWidget : rack::widget::TransparentWidget {
        void draw (const DrawArgs& args) override;
        void drawLayer (const DrawArgs& args, int layer) override;
    };

    struct MetaHandler {
        typedef int64_t TimeUnit;

    private:
        TimeUnit curTime;

        // Meta module
        uint16_t metaModuleCount = 0;

        // Cable data
        std::size_t cables_prevCount = 0;
        bool cables_hadIncomplete = false;

        bool cables_Connected = false;
        bool cables_Disconnected = false;

        MetaCableWidget* metaCableWidget = nullptr;

        // Module data
        std::unordered_map<RackModuleId, TimeUnit> modules_Time;

        bool modules_Added = false;
        bool modules_Removed = false;
        bool modules_AnyAdded = false;
        bool modules_AnyRemoved = false;

        MetaHandler ();
        ~MetaHandler ();

        MetaHandler (const MetaHandler& x) = delete;
        void operator= (const MetaHandler& x) = delete;

        void update ();
        void updateCables ();
        void updateModules ();

    public:
        static std::shared_ptr<MetaHandler> getHandler ();

        // Meta module
        bool hasMetaModule () { return metaModuleCount != 0; }
        void addMetaModule () { metaModuleCount++; }
        void removeMetaModule () { metaModuleCount--; }

        // Cable data
        bool checkCableConnected () { return cables_Connected; }
        bool checkCableDisconnected () { return cables_Disconnected; }

        // Module data
        bool checkModuleAdded () { return modules_Added; }
        bool checkModuleRemoved () { return modules_Removed; }
    };
}