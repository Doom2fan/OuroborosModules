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

#include "AutomataCommon.hpp"
#include "Automata.hpp"

#include "../JsonUtils.hpp"

namespace OuroborosModules::Modules::Automata {
    /*
     * Preset rules
     */
    std::pair<std::string, AutomataRules> calculatePresetRule (
        std::string name,
        std::vector<uint8_t> birth, std::vector<uint8_t> survival
    ) {
        auto rules = AutomataRules ();

        auto lastNumber = -1;
        for (auto idx : birth) {
            if (idx > 8) {
                LOG_FATAL (FMT_STRING ("Error in preset rule \"{}\" (B): Index #{} > 8"), name, idx);
                goto error;
            } else if (idx == lastNumber) {
                LOG_FATAL (FMT_STRING ("Error in preset rule \"{}\" (B): Repeated index #{}"), name, idx);
                goto error;
            } else if (idx < lastNumber) {
                LOG_FATAL (FMT_STRING ("Error in preset rule \"{}\" (B): Out of order index #{}"), name, idx);
                goto error;
            }

            rules.setBirthFlag (idx, true);
        }

        lastNumber = -1;
        for (auto idx : survival) {
            if (idx > 8) {
                LOG_FATAL (FMT_STRING ("Error in preset rule \"{}\" (S): Index #{} > 8"), name, idx);
                goto error;
            } else if (idx == lastNumber) {
                LOG_FATAL (FMT_STRING ("Error in preset rule \"{}\" (S): Repeated index #{}"), name, idx);
                goto error;
            } else if (idx < lastNumber) {
                LOG_FATAL (FMT_STRING ("Error in preset rule \"{}\" (S): Out of order index #{}"), name, idx);
                goto error;
            }

            rules.setSurvivalFlag (idx, true);
        }

        return std::make_pair (name, rules);

    error:
        return std::make_pair ("#ERROR", AutomataRules ());
    }

    std::vector<std::pair<std::string, AutomataRules>> presetRules = {
        calculatePresetRule ("Conway's Game of Life (Default)", { 3 }, { 2, 3 }),
        calculatePresetRule ("34 Life", { 3, 4 }, { 3, 4, }),
        calculatePresetRule ("HighLife", { 3, 6 }, { 2, 3 }),
        calculatePresetRule ("Seeds", { 2 }, { }),
        calculatePresetRule ("Replicator", { 1, 3, 5, 7 }, { 1, 3, 5, 7 }),
        calculatePresetRule ("Diamoeba", { 3, 5, 6, 7, 8 }, { 5, 6, 7, 8 }),
        calculatePresetRule ("2x2", { 3, 6 }, { 1, 2, 5 }),
        calculatePresetRule ("Day & Night", { 3, 6, 7, 8 }, { 3, 4, 6, 7, 8 }),
        calculatePresetRule ("Morley/Move", { 3, 6, 8 }, { 2, 4, 5 }),
        calculatePresetRule ("Anneal", { 4, 6, 7, 8 }, { 3, 5, 6, 7, 8 }),
    };

    /*
     * AutomataCell
     */
    AutomataMode modeFromSelectorParam (float paramVal) {
        return static_cast<AutomataMode> (std::clamp (paramVal, -2.f, TriggerCount - 1.f));
    }

    AutomataCell modeToCellTrigger (AutomataMode mode) {
        if (mode < AutomataMode::EditTrigger || mode > AutomataMode::EditTrigger_LAST)
            return AutomataCell::None;

        return static_cast<AutomataCell> (1 << (8 + static_cast<int> (mode)));
    }

    int triggerFromCell (AutomataCell cell) {
        switch (cell) {
            default: return -1;

            case AutomataCell::TRIGGER_1: return 1;
            case AutomataCell::TRIGGER_2: return 2;
            case AutomataCell::TRIGGER_3: return 3;
            case AutomataCell::TRIGGER_4: return 4;
            case AutomataCell::TRIGGER_5: return 5;
            case AutomataCell::TRIGGER_6: return 6;
            case AutomataCell::TRIGGER_7: return 7;
            case AutomataCell::TRIGGER_8: return 8;
        }
    }

    /*
     * AutomataRules
     */
    bool AutomataRules::getFlag (RuleMaskType maskField, int idx) const {
        assert (idx >= 0 && idx < NeighborsCount);
        if (idx < 0 || idx >= NeighborsCount)
            return false;

        return (maskField & (1 << idx)) != 0;
    }

    void AutomataRules::setFlag (RuleMaskType& maskField, int idx, bool set) {
        assert (idx >= 0 && idx < NeighborsCount);
        if (idx < 0 || idx >= NeighborsCount)
            return;

        auto flagMask = 1 << idx;
        maskField = (maskField & ~flagMask) | (set ? flagMask : 0);
    }

    std::string AutomataRules::getRuleString () {
        // Size = B012345678/S012345678 + extra byte
        static constexpr int MaxLength = 22;
        struct TextBuffer {
            char buffer [MaxLength];
            std::size_t textLen = 0;

            void append (char c) {
                if (textLen < MaxLength)
                    buffer [textLen++] = c;
            }

            std::string flush () { return std::string (buffer, textLen); }
        };

        auto textBuf = TextBuffer ();

        textBuf.append ('B');
        for (int i = 0; i < NeighborsCount; i++) {
            auto neighbourIdx = 1 << i;
            if ((birthMask & neighbourIdx) != 0)
                textBuf.append ('0' + i);
        }
        textBuf.append ('/');
        textBuf.append ('S');
        for (int i = 0; i < NeighborsCount; i++) {
            auto neighbourIdx = 1 << i;
            if ((survivalMask & neighbourIdx) != 0)
                textBuf.append ('0' + i);
        }

        return textBuf.flush ();
    }

    json_t* AutomataRules::dataToJson () const {
        auto rootJ = json_object ();

        json_object_set_new_int (rootJ, "birthMask", birthMask);
        json_object_set_new_int (rootJ, "survivalMask", survivalMask);

        return rootJ;
    }

    bool AutomataRules::dataFromJson (json_t* rootJ) {
        if (!json_is_object (rootJ))
            return false;

        json_object_try_get_int (rootJ, "birthMask", birthMask);
        json_object_try_get_int (rootJ, "survivalMask", survivalMask);

        return true;
    }

    /*
     * AutomataTriggerInfo
     */
    json_t* AutomataTriggerInfo::dataToJson () const {
        auto rootJ = json_object ();

        json_object_set_new_enum (rootJ, "countMode", countMode);
        json_object_set_new_enum (rootJ, "outputMode", outputMode);

        return rootJ;
    }

    bool AutomataTriggerInfo::dataFromJson (json_t* rootJ) {
        if (!json_is_object (rootJ))
            return false;

        json_object_try_get_enum (rootJ, "countMode", countMode);
        json_object_try_get_enum (rootJ, "outputMode", outputMode);

        return true;
    }

    /*
     * Default board
     */
    int DefaultBoard_Seed [BoardHeight] [BoardWidth] = {
        { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, },
        { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, },
        { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, },
        { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, },
        { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, },
        { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, },
        { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, },
        { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, },
        { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, },
        { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, },
        { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, },
        { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, },
        { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, },
        { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, },
        { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, },
        { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, },
        { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, },
        { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, },
        { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, },
        { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, },
    };
}