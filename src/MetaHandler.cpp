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

#include "MetaHandler.hpp"

#include "UISystemUpdater.hpp"
#include "Utils.hpp"

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

            curTime = 0;
            initialized = true;
        }
        UISystemUpdater::tryCreate ();
    }

    MetaHandler::~MetaHandler () {
        if (metaCableWidget != nullptr) {
            if (metaCableWidget->parent != nullptr)
                metaCableWidget->parent->removeChild (metaCableWidget);

            delete metaCableWidget;
        }

        if (APP != nullptr &&
            APP->scene != nullptr &&
            APP->scene->rack != nullptr &&
            APP->scene->rack->getCableContainer () != nullptr)
            APP->scene->rack->getCableContainer ()->show ();
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
        updateModules ();
        updateCables ();

        curTime++;
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

        modules_AnyAdded   = modulesAdded > 0;
        modules_AnyRemoved = modulesRemoved > 0;
        modules_Added   = modulesAdded == 1 && modulesRemoved == 0;
        modules_Removed = modulesAdded == 0 && modulesRemoved == 1;
    }
}