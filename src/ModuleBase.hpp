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

#include <sst/rackhelpers/neighbor_connectable.h>

namespace OuroborosModules {
    using SST_NeighborConnectable_V1 = sst::rackhelpers::module_connector::NeighborConnectable_V1;

    struct ModuleBase : rack::engine::Module {
      public:
        ThemeId theme_Override = ThemeId::getUnknown ();
        EmblemId theme_Emblem = EmblemId::getUnknown ();

        json_t* dataToJson () override;
        void dataFromJson (json_t* rootJ) override;

        template <class TParamQuantity = rack::engine::ParamQuantity>
        TParamQuantity* configParamSnap (
            int paramId,
            float minValue, float maxValue, float defaultValue,
            std::string name = "", std::string unit = "",
            float displayBase = 0.f, float displayMultiplier = 1.f, float displayOffset = 0.f
        ) {
            assert (paramId < (int) params.size () && paramId < (int) paramQuantities.size ());

            auto quantity = configParam<TParamQuantity> (
                paramId,
                minValue, maxValue, defaultValue,
                name, unit,
                displayBase, displayMultiplier, displayOffset
            );
            quantity->ParamQuantity::snapEnabled = true;
            quantity->ParamQuantity::smoothEnabled = false;

            return quantity;
        }
    };
}