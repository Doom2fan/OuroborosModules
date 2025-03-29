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

#pragma once

#include <fmt/format.h>

#define LOG_DEBUG(fmt, ...) OuroborosModules::Logging::log (OuroborosModules::Logging::LogLevel::Debug, __FILE__, __LINE__, __FUNCTION__, fmt, ##__VA_ARGS__)
#define LOG_INFO(fmt, ...) OuroborosModules::Logging::log (OuroborosModules::Logging::LogLevel::Info, __FILE__, __LINE__, __FUNCTION__, fmt, ##__VA_ARGS__)
#define LOG_WARN(fmt, ...) OuroborosModules::Logging::log (OuroborosModules::Logging::LogLevel::Warn, __FILE__, __LINE__, __FUNCTION__, fmt, ##__VA_ARGS__)
#define LOG_FATAL(fmt, ...) OuroborosModules::Logging::log (OuroborosModules::Logging::LogLevel::Fatal, __FILE__, __LINE__, __FUNCTION__, fmt, ##__VA_ARGS__)

namespace OuroborosModules::Logging {
    enum class LogLevel {
        Debug,
        Info,
        Warn,
        Fatal,
    };

    void logInternal (
        LogLevel level,
        const char* file, int line, const char* function,
        fmt::string_view fmt, fmt::format_args args
    );

    template<typename... T>
    void log (
        LogLevel level,
        const char* file, int line, const char* function,
        fmt::format_string<T...> fmt, T&&... args
    ) {
        logInternal (level, file, line, function, fmt, fmt::make_format_args (args...));
    }
}