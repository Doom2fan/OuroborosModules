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

#include "UISystemUpdater.hpp"

namespace {
    static UISystemUpdater* currentUpdater = nullptr;
}

void UISystemUpdater::tryCreate () {
    if (currentUpdater != nullptr)
        return;

    auto widget = new UISystemUpdater;
    APP->scene->rack->addChild (widget);
}

void UISystemUpdater::addUpdateFunction (std::function<void()> func) {
    updateFunctions.push_back (func);
}

UISystemUpdater::UISystemUpdater () {
    if (currentUpdater == nullptr)
        currentUpdater = this;

    box.pos = box.size = rack::math::Vec ();
}

UISystemUpdater::~UISystemUpdater () {
    if (currentUpdater == this)
        currentUpdater = nullptr;
}

void UISystemUpdater::step () {
    TransparentWidget::step ();

    for (auto func : updateFunctions)
        func ();
}