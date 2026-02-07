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

#include "Common.hpp"

#include "../PluginDef.hpp"

#include <algorithm>

namespace OuroborosModules::Modules::Conductor {
    static constexpr int HeartbeatTime = 256;
    static constexpr int CoreHeartbeatTime = HeartbeatTime - 4;

    int patternFloatToInt (float v) {
        return static_cast<int> (std::round (v));
    }

    /*
     * ConductorCore
     */
    template<typename Func>
    void iterateExpanders (rack::engine::Module* thisModule, Func&& func) {
        auto initialModule = thisModule->getRightExpander ().module;
        for (auto module = initialModule; module != nullptr; module = module->getRightExpander ().module) {
            if (module == thisModule)
                break;

            auto expander = dynamic_cast<ConductorExpander*> (module);
            if (expander == nullptr)
                break;

            func (expander);
        }
    }

    void ConductorCore::processCore () {
        if (heartbeatTimer > 0) {
            heartbeatTimer--;
            return;
        }

        heartbeatTimer = CoreHeartbeatTime;
        iterateExpanders (this, [this] (ConductorExpander* expander) {
            expander->heartbeat (this);
        });
    }

    void ConductorCore::emitOnDataUpdated () {
        ConductorDataUpdatedEvent e;
        prepareDataUpdatedEvent (e);

        iterateExpanders (this, [this, &e] (ConductorExpander* expander) {
            expander->heartbeat (this);
            expander->onDataUpdated (e);
        });
    }

    void ConductorCore::expanderConnected (ConductorExpander* expander) {
        ConductorDataUpdatedEvent e;
        prepareDataUpdatedEvent (e);

        expander->heartbeat (this);
        expander->onDataUpdated (e);
    }

    void ConductorCore::onRemove (const RemoveEvent& e) {
        ModuleBase::onRemove (e);

        iterateExpanders (this, [this] (ConductorExpander* expander) {
            expander->onCoreRemoved (this);
        });
    }

    /*
     * ConductorExpander
     */
    void ConductorExpander::processExpander () {
        if (heartbeatTimer > 0 && --heartbeatTimer < 1)
            coreModule = nullptr;

        if (tryFindCore) {
            tryFindCore = false;

            for (auto module = getLeftExpander ().module; module != nullptr; module = module->getLeftExpander ().module) {
                auto expander = dynamic_cast<ConductorExpander*> (module);

                if (expander == nullptr) {
                    if (auto core = dynamic_cast<ConductorCore*> (module))
                        core->expanderConnected (this);

                    break;
                }
            }
        }
    }

    void ConductorExpander::heartbeat (ConductorCore* core) {
        heartbeatTimer = HeartbeatTime;
        coreModule = core;
    }

    void ConductorExpander::onCoreRemoved (ConductorCore* core) {
        if (coreModule != core)
            return;

        heartbeatTimer = 0;
        coreModule = nullptr;
    }

    void ConductorExpander::onExpanderChange (const ExpanderChangeEvent& e) {
        ModuleBase::onExpanderChange (e);

        tryFindCore = true;
    }
}