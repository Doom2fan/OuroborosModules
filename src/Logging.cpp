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

#include "Logging.hpp"

#include "PluginDef.hpp"

namespace OuroborosModules::Logging {
    //rack::logger::log(rack::logger::DEBUG_LEVEL, __FILE__, __LINE__, __FUNCTION__, format, ##__VA_ARGS__)
    rack::logger::Level getRackLevel (LogLevel level) {
        switch (level) {
            case LogLevel::Debug: return rack::logger::DEBUG_LEVEL;
            case LogLevel::Info: return rack::logger::INFO_LEVEL;
            case LogLevel::Warn: return rack::logger::WARN_LEVEL;
            case LogLevel::Fatal: return rack::logger::FATAL_LEVEL;
        }

        return rack::logger::DEBUG_LEVEL;
    }

    void logInternal (
        LogLevel level,
        const char* file, int line, const char* function,
        fmt::string_view fmt, fmt::format_args args
    ) {
        if (pluginSettings.debug_Logging)
            rack::logger::log (getRackLevel (level), file, line, function, "%s", fmt::vformat (fmt, args).c_str ());
    }
}