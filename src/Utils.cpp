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

#include "Utils.hpp"

#include <osdialog.h>

namespace OuroborosModules::Hashing {
    std::size_t distribute (const std::size_t& n) {
        static constexpr auto digits = std::numeric_limits<std::size_t>::digits;
        static_assert (digits == 64 || digits == 32);

        if (digits == 64) {
            uint64_t p = 0x5555555555555555ull; // pattern of alternating 0 and 1
            uint64_t c = 17316035218449499591ull; // random uneven integer constant
            return static_cast<size_t> (c * xorshift (p * xorshift (n, 32), 32));
        } else {
            uint32_t p = 0x55555555ul; // pattern of alternating 0 and 1
            uint32_t c = 3423571495ul; // random uneven integer constant
            return static_cast<size_t> (c * xorshift (p * xorshift (n, 16), 16));
        }

        assert (false);
        return 0;
    }
}

namespace OuroborosModules {
    char* selectSoundFile () {
        static const char FILE_FILTERS [] = "Wave (.wav):wav,WAV;All files (*.*):*.*";
        auto filters = osdialog_filters_parse (FILE_FILTERS);
        DEFER ({ osdialog_filters_free (filters); });

        return osdialog_file (OSDIALOG_OPEN, nullptr, nullptr, filters);
    }
}