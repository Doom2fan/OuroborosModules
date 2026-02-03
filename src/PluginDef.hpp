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

#include "Constants.hpp"
#include "PluginSettings.hpp"
#include "Logging.hpp"
#include "ModelDeclarations.hpp"

#include <cstddef>
#include <cstdint>

#include <algorithm>
#include <array>
#include <vector>

#include <rack.hpp>

// Usings
using rack::app::RACK_GRID_WIDTH;
using rack::app::RACK_GRID_HEIGHT;

// Typedefs
typedef int64_t RackModuleId;

// Externs
extern rack::plugin::Plugin* pluginInstance;