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

#include "CableHandler.hpp"

namespace {
    static std::weak_ptr<CableHandler> currentHandler;
    static CableHandlerWidget* currentWidget = nullptr;

    struct CableHandlerWidget : rack::widget::TransparentWidget {
        static void tryCreate () {
            if (currentWidget != nullptr)
                return;

            auto widget = new CableHandlerWidget;
            APP->scene->rack->addChild (widget);
        }

        CableHandlerWidget () {
            if (currentWidget == nullptr)
                currentWidget = this;

            box.pos = box.size = rack::math::Vec ();
        }

        ~CableHandlerWidget () {
            if (currentWidget == this)
                currentWidget = nullptr;
        }

        void step () override {
            if (auto handler = currentHandler.lock ())
                handler->update ();
        }
    };
}

CableHandler::CableHandler () {
    CableHandlerWidget::tryCreate ();
}

std::shared_ptr<CableHandler> CableHandler::getHandler () {
    struct CableHandler_Concrete : public CableHandler { };

    if (auto retVal = currentHandler.lock ())
        return retVal;

    auto retVal = std::make_shared<CableHandler_Concrete> ();
    currentHandler = retVal;
    return retVal;
}

void CableHandler::update () {
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

    cableConnected = cableDisconnected = false;

    if (hasIncompleteCable && !hadIncompleteCable) {
        if (cableCount == prevCableCount)
            cableConnected = true;
        else if (cableCount < prevCableCount)
            cableDisconnected = true;
    } else if (!hasIncompleteCable && hadIncompleteCable) {
        if (cableCount > prevCableCount)
            cableConnected = true;
        else
            cableDisconnected = true;
    }

    prevCableCount = cableCount;
    hadIncompleteCable = hasIncompleteCable;
}