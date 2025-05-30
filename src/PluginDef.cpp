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

#include "PluginDef.hpp"

#include "ModelDeclarations.hpp"

#include <fmt/format.h>
#include <rack_themer.hpp>

#include <string>

rack::plugin::Plugin* pluginInstance;

namespace OuroborosModules::Modules::Meta {
    void metaSounds_Init ();
    void metaSounds_Refresh ();
}

void rackThemerLogger (rack_themer::logging::Severity severity, rack_themer::logging::ErrorCode code, std::string info) {
    using namespace rack_themer::logging;

    if (!pluginSettings.debug_Logging)
        return;

    auto message = fmt::format (FMT_STRING ("[vcv-rackthemer: {}] {}"), severityName (severity), info);
    switch (severity) {
        default:
        case Severity::Info:
            INFO ("%s", message.c_str ());
            break;

        case Severity::Warn:
        case Severity::Error:
        case Severity::Critical:
            WARN ("%s", message.c_str ());
            break;
    }
}

__attribute__((__visibility__("default"))) void init (rack::plugin::Plugin* p) {
    pluginInstance = p;

    rack_themer::logging::setLogger (&rackThemerLogger);

    OuroborosModules::initSettings ();
    OuroborosModules::Modules::Meta::metaSounds_Init ();

    // Module models.
    init_InitializeModels (p);
}

__attribute__((__visibility__("default"))) json_t* settingsToJson () {
    return pluginSettings.saveToJson ();
}

__attribute__((__visibility__("default"))) void settingsFromJson (json_t* rootJ) {
    pluginSettings.readFromJson (rootJ);
    OuroborosModules::Modules::Meta::metaSounds_Refresh ();
}