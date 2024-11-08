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

#include "MetaHandler.hpp"

#include "UISystemUpdater.hpp"

namespace {
    static bool initialized;
    static std::weak_ptr<OuroborosModules::MetaHandler> currentHandler;
}

namespace OuroborosModules {
    MetaHandler::MetaHandler () {
        if (!initialized) {
            UISystemUpdater::addUpdateFunction ([] () {
                if (auto handler = currentHandler.lock ())
                    handler->update ();
            });
            initialized = true;
        }
        UISystemUpdater::tryCreate ();
    }

    std::shared_ptr<MetaHandler> MetaHandler::getHandler () {
        struct MetaHandler_Concrete : public MetaHandler { };

        if (auto retVal = currentHandler.lock ())
            return retVal;

        auto retVal = std::make_shared<MetaHandler_Concrete> ();
        currentHandler = retVal;
        return retVal;
    }

    void MetaHandler::update () {
        updateCables ();
        updateModules ();

        curTime++;
    }

    void MetaHandler::updateCables () {
        auto cableContainer = APP->scene->rack->getCableContainer ();
        auto incompleteCable = APP->scene->rack->getIncompleteCable ();

        if (cableContainer == nullptr)
            return;

        auto hasIncompleteCable = incompleteCable != nullptr;
        int cableCount = 0;

        for (auto it = cableContainer->children.begin (), it_end = cableContainer->children.end (); it != it_end; ++it) {
            auto cable = dynamic_cast<rack::app::CableWidget*> (*it);
            if (cable == nullptr || !cable->isComplete ())
                continue;

            cableCount++;
        }

        cables_Connected = cables_Disconnected = false;

        if (hasIncompleteCable && !cables_hadIncomplete) {
            if (cableCount == cables_prevCount)
                cables_Connected = true;
            else if (cableCount < cables_prevCount)
                cables_Disconnected = true;
        } else if (!hasIncompleteCable && cables_hadIncomplete) {
            if (cableCount > cables_prevCount)
                cables_Connected = true;
            else
                cables_Disconnected = true;
        }

        cables_prevCount = cableCount;
        cables_hadIncomplete = hasIncompleteCable;
    }

    void MetaHandler::updateModules () {
        modules_Added = false;
        modules_Removed = false;

        int modulesAdded = 0;
        int modulesRemoved = 0;

        const int bufferSize = 64;
        RackModuleId moduleIds [bufferSize];
        for (int moduleCount = APP->engine->getNumModules (); moduleCount > 0;) {
            auto returnedCount = APP->engine->getModuleIds (moduleIds, bufferSize);

            for (std::size_t i = 0; i < returnedCount; i++) {
                auto moduleId = moduleIds [i];
                auto search = modules_Time.find (moduleId);
                if (search == modules_Time.end ()) {
                    modules_Time [moduleId] = curTime;
                    modulesAdded++;
                } else
                    search->second = curTime;
            }

            moduleCount -= returnedCount;
        }

        for (auto it = modules_Time.begin (); it != modules_Time.end ();) {
            if (it->second != curTime){
                it = modules_Time.erase (it);
                modulesRemoved++;
            } else
                it++;
        }


        modules_Added   = modulesAdded == 1 && modulesRemoved == 0;
        modules_Removed = modulesAdded == 0 && modulesRemoved == 1;
    }
}