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

#include <array>
#include <atomic>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace OuroborosModules::Modules::Automata {
    struct AutomataModule;

    struct AutomataLife;
    struct AutomataBoard;
    struct AutomataRules;

    struct AutomataWidget;
    struct AutomataRulesWidget;
    struct AutomataBoardWidget;
    struct AutomataRulesWidgetBitButton;

    using AutomataLifeUpdateIndex = uint32_t;

    static constexpr int BoardWidth = 32;
    static constexpr int BoardHeight = 20;
    static constexpr int TriggerCount = 8;
    static constexpr int MaxSequenceLength = 64;
    static constexpr int NeighborsCount = 9;

    static constexpr float BoardMargin = 8;
    static constexpr float BoardSpacing = 1;

    extern std::vector<std::pair<std::string, AutomataRules>> presetRules;

    /*
     * AutomataCell
     */
    using AutomataCellBase = uint16_t;
    enum class AutomataCell : AutomataCellBase {
        None = 0,

        FLAG_LiveA = 1 << 0,
        FLAG_LiveB = 1 << 1,
        FLAG_SeedSet = 1 << 2,

        TRIGGER_1 = 1 << 8,
        TRIGGER_2 = 1 << 9,
        TRIGGER_3 = 1 << 10,
        TRIGGER_4 = 1 << 11,
        TRIGGER_5 = 1 << 12,
        TRIGGER_6 = 1 << 13,
        TRIGGER_7 = 1 << 14,
        TRIGGER_8 = 1 << 15,

        MASK_Live = FLAG_LiveA | FLAG_LiveB,
        MASK_Triggers = TRIGGER_1 | TRIGGER_2 | TRIGGER_3 | TRIGGER_4 |
                        TRIGGER_5 | TRIGGER_6 | TRIGGER_7 | TRIGGER_8,
    };

    inline AutomataCell operator| (const AutomataCell lhs, const AutomataCell rhs) {
        return static_cast<AutomataCell> (static_cast<AutomataCellBase> (lhs) | static_cast<AutomataCellBase> (rhs));
    }
    inline AutomataCell operator& (const AutomataCell lhs, const AutomataCell rhs) {
        return static_cast<AutomataCell> (static_cast<AutomataCellBase> (lhs) & static_cast<AutomataCellBase> (rhs));
    }
    inline AutomataCell operator^ (const AutomataCell lhs, const AutomataCell rhs) {
        return static_cast<AutomataCell> (static_cast<AutomataCellBase> (lhs) ^ static_cast<AutomataCellBase> (rhs));
    }
    inline AutomataCell operator~ (AutomataCell x) { return static_cast<AutomataCell> (~static_cast<AutomataCellBase> (x)); }
    inline AutomataCell& operator|= (AutomataCell& lhs, const AutomataCell& rhs) { return lhs = lhs | rhs; }
    inline AutomataCell& operator&= (AutomataCell& lhs, const AutomataCell& rhs) { return lhs = lhs & rhs; }
    inline AutomataCell& operator^= (AutomataCell& lhs, const AutomataCell& rhs) { return lhs = lhs ^ rhs; }

    inline bool testCellFlag (AutomataCell cell, AutomataCell flags) { return (cell & flags) == flags; }
    inline bool testCellFlagAny (AutomataCell cell, AutomataCell flags) { return (cell & flags) != AutomataCell::None; }
    inline AutomataCellBase extractCellTriggers (AutomataCell cell) {
        return static_cast<AutomataCellBase> (cell & AutomataCell::MASK_Triggers) >> 8;
    }
    inline AutomataCell cellFromTriggerIndex (AutomataCellBase triggerIndex) {
        return triggerIndex < TriggerCount ? static_cast<AutomataCell> (1 << (8 + triggerIndex)) : AutomataCell::None;
    }
    int triggerFromCell (AutomataCell cell);

    /*
     * AutomataMode
     */
    enum class AutomataMode {
        Play = -2,
        EditSeed = -1,
        ENUMS (EditTrigger, TriggerCount),
    };
    AutomataMode modeFromSelectorParam (float paramVal);
    AutomataCell modeToCellTrigger (AutomataMode mode);

    /*
     * AutomataRules
     */
    using RuleMaskType = uint16_t;
    struct AutomataRules {
        static constexpr RuleMaskType BitMask = (1 << (NeighborsCount)) - 1;
      private:
        RuleMaskType birthMask;
        RuleMaskType survivalMask;

        bool getFlag (RuleMaskType maskField, int idx) const;
        void setFlag (RuleMaskType& maskField, int idx, bool set);

      public:
        AutomataRules () : birthMask (0), survivalMask (0) { }

        RuleMaskType getBirthMask () const { return birthMask; }
        RuleMaskType getSurvivalMask () const { return survivalMask; }

        bool getBirthFlag (int idx) const { return getFlag (birthMask, idx); }
        void setBirthFlag (int idx, bool set) { setFlag (birthMask, idx, set); }

        bool getSurvivalFlag (int idx) const { return getFlag (survivalMask, idx); }
        void setSurvivalFlag (int idx, bool set) { setFlag (survivalMask, idx, set); }

        std::string getRuleString ();

        json_t* dataToJson () const;
        bool dataFromJson (json_t* rootJ);

        bool operator== (const AutomataRules& rhs) const {
            return (birthMask    & BitMask) == (rhs.birthMask    & BitMask) &&
                   (survivalMask & BitMask) == (rhs.survivalMask & BitMask);
        }
        bool operator!= (const AutomataRules& rhs) const { return !(*this == rhs); }
    };

    /*
     * AutomataTriggerInfo
     */
    enum class AutomataTriggerCountMode : int {
        Newborn,
        IsAlive,
    };

    enum class AutomataTriggerOutputMode : int {
        Trigger,
        Percentage,
    };

    struct AutomataTriggerInfo {
      public:
        AutomataTriggerCountMode countMode = AutomataTriggerCountMode::Newborn;
        AutomataTriggerOutputMode outputMode = AutomataTriggerOutputMode::Trigger;

        json_t* dataToJson () const;
        bool dataFromJson (json_t* rootJ);
    };

    /*
     * Default board
     */
    extern int DefaultBoard_Seed [BoardHeight] [BoardWidth];
}