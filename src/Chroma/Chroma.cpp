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

#include "Chroma.hpp"

#include "../JsonUtils.hpp"
#include "CCM_Common.hpp"

namespace OuroborosModules {
    rack::plugin::Model* modelChroma = createModel<Modules::Chroma::ChromaWidget> ("CableColorModule");
}

namespace OuroborosModules::Modules::Chroma {
    ChromaModule::ChromaModule () {
        colorManager = getColorManager ();
        checkMaster ();
    }

    bool ChromaModule::checkMaster () {
        if (masterModule == nullptr)
            masterModule = this;

        return masterModule == this;
    }

    json_t* ChromaModule::dataToJson () {
        auto rootJ = ModuleBase::dataToJson ();

        json_object_set_new_bool (rootJ, "wasMaster", checkMaster ());
        json_object_set_new_enum (rootJ, "centerEmblem", centerEmblem);

        auto colorManagerJ = colorManager->dataToJson ();
        json_object_set_new (rootJ, "colorManager", colorManagerJ);

        return rootJ;
    }

    void ChromaModule::dataFromJson (json_t* rootJ) {
        ModuleBase::dataFromJson (rootJ);

        if (!json_is_object (rootJ))
            return;

        json_object_try_get_enum (rootJ, "centerEmblem", centerEmblem);

        if (!json_is_true (json_object_get (rootJ, "wasMaster")))
            return;

        if (colorManager != nullptr) {
            auto colorManagerJ = json_object_get (rootJ, "colorManager");
            colorManager->dataFromJson (colorManagerJ);
        }
    }
}