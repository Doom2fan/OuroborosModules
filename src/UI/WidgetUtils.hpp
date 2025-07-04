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

#include "../PluginDef.hpp"

namespace OuroborosModules::Widgets {
    template<class TModuleLightWidget, typename... TArgs>
    TModuleLightWidget* createLight (
        rack::math::Vec pos,
        rack::engine::Module* module, int firstLightId,
        TArgs &&...args
    ) {
        TModuleLightWidget* o = new TModuleLightWidget (std::forward<TArgs> (args) ...);
        o->box.pos = pos;
        o->rack::app::ModuleLightWidget::module = module;
        o->rack::app::ModuleLightWidget::firstLightId = firstLightId;
        return o;
    }

    template<class TModuleLightWidget, typename... TArgs>
    TModuleLightWidget* createLightCentered (
        rack::math::Vec pos,
        rack::engine::Module* module, int firstLightId,
        TArgs &&...args
    ) {
        TModuleLightWidget* o = createLight<TModuleLightWidget> (pos, module, firstLightId, std::forward<TArgs> (args) ...);
        o->box.pos = o->box.pos.minus (o->box.size.div (2));
        return o;
    }

    template<class TWidget, typename... TArgs>
    TWidget* createWidget (rack::math::Vec pos, TArgs &&...args) {
        TWidget* o = new TWidget (std::forward<TArgs> (args) ...);
        o->box.pos = pos;
        return o;
    }

    template<class TWidget, typename... TArgs>
    TWidget* createWidgetCentered (rack::math::Vec pos, TArgs &&...args) {
        TWidget* o = new TWidget (std::forward<TArgs> (args) ...);
        o->box.pos = pos.minus (o->box.size.div (2));
        return o;
    }
}