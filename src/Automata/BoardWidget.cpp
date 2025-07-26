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

#include "Automata.hpp"

#include "../UI/WidgetUtils.hpp"

#include <fmt/format.h>

namespace OuroborosModules::Modules::Automata {
    struct BoardCoords {
        rack::math::Vec gridSize;
        rack::math::Vec cellCounts;
        rack::math::Vec spacingLosses;
        rack::math::Vec cellSize;

        BoardCoords (AutomataBoardWidget* boardWidget, float footerSize) {
            gridSize = boardWidget->box.size;
            gridSize.x -= BoardMargin * 2;
            gridSize.y -= BoardMargin * 2 + 4 + footerSize;

            cellCounts = rack::math::Vec (BoardWidth, BoardHeight);
            spacingLosses = BoardSpacing * cellCounts.minus (1);
            cellSize = gridSize.minus (spacingLosses).div (cellCounts);
        }

        rack::math::Vec getCellPos (int x, int y) const {
            return rack::math::Vec (x * (cellSize.x + BoardSpacing), y * (cellSize.y + BoardSpacing));
        }

        bool posToCell (rack::math::Vec pos, int& x, int& y) const {
            pos = pos.minus (BoardMargin);

            // Outside the grid.
            if (pos.x < 0 || pos.x > gridSize.x || pos.y < 0 || pos.y > gridSize.y) {
                x = -1; y = -1;
                return false;
            }

            auto fullCellSize = cellSize.plus (BoardSpacing);
            auto col = pos.x / fullCellSize.x;
            auto row = pos.y / fullCellSize.y;

            if (col >= BoardWidth || row >= BoardHeight) {
                x = -1; y = -1;
                return false;
            }

            auto fullCellX = pos.x - (col * fullCellSize.x);
            auto fullCellY = pos.y - (row * fullCellSize.y);

            if (fullCellX > cellSize.x || fullCellY > cellSize.y) {
                x = -1; y = -1;
                return false;
            }

            x = col;
            y = row;
            return true;
        }
    };

    AutomataBoardWidget::AutomataBoardWidget (rack::math::Vec size, AutomataWidget* panelWidget)
        : editGrid () {
        using rack::math::Vec;

        this->panelWidget = panelWidget;
        box.size = size;

        auto displayBG = rack::createWidget<rack::LedDisplay> (Vec ());
        displayBG->box.size = size;
        addChild (displayBG);

        auto module = panelWidget->getAutomata ();
        if (module != nullptr)
            module->lifeBoard.checkUpdated (lifeUpdateIndex);
        else { // Module browser
            // Default rule (Game of Life)
            ruleString = "B3/S23";

            // Default seed
            internalBoard.clear ();
            for (int y = 0; y < BoardHeight; y++) {
                for (int x = 0; x < BoardWidth; x++) {
                    if (DefaultBoard_Seed [y] [x] == 1)
                        internalBoard.at (x, y) = AutomataCell::FLAG_LiveA;
                }
            }
        }
    }

    void AutomataBoardWidget::step () {
        _ThemedWidgetBase::step ();

        auto module = panelWidget->getAutomata ();
        if (module == nullptr)
            return;

        auto curRules = module->getLifeBoard ().getRules ();
        if (lastRules != curRules) {
            ruleString = curRules.getRuleString ();
            lastRules = curRules;
        }

        if (!module->lifeBoard.checkUpdated (lifeUpdateIndex))
            return;

        internalBoard = module->lifeBoard.getBoard ();
        for (auto it = editCommands.begin (); it != editCommands.end ();) {
            if ((*it)->executed) {
                it = editCommands.erase (it);
                continue;
            }

            (*it)->execute (internalBoard);

            ++it;
        }
    }

    auto getRulesFont () {
        return APP->window->loadFont (rack::asset::plugin (pluginInstance, "res/fonts/Doto-Black.ttf"));
    }

    void AutomataBoardWidget::draw (const DrawArgs& args) {
        if (panelWidget->getDisplayLayer () == 0)
            drawBoard (args);

        _ThemedWidgetBase::draw (args);
    }

    void AutomataBoardWidget::drawLayer (const DrawArgs& args, int layer) {
        using rack::math::Vec;

        if (layer == panelWidget->getDisplayLayer ())
            drawBoard (args);

        _ThemedWidgetBase::drawLayer (args, layer);
    }

    void AutomataBoardWidget::drawBoard (const DrawArgs& args) {
        using rack::math::Vec;

        if (panelWidget->getRulesWidget ()->isOpen ())
            return;

        auto module = panelWidget->getAutomata ();

        nvgSave (args.vg);

        // Rule display
        auto rulesFont = getRulesFont ();
        nvgFontFaceId (args.vg, getRulesFont ()->handle);
        nvgFontSize (args.vg, 16);
        nvgTextLetterSpacing (args.vg, 0);
        nvgTextLineHeight (args.vg, 1);

        std::string_view textStr = ruleString;
        auto ruleStringPos = Vec (BoardMargin, box.size.y - BoardMargin);
        nvgFillColor (args.vg, rack::color::WHITE);
        nvgTextAlign (args.vg, NVG_ALIGN_LEFT | NVG_ALIGN_BOTTOM);
        nvgText (args.vg, VEC_ARGS (ruleStringPos), textStr.begin (), textStr.end ());
        nvgTextMetrics (args.vg, nullptr, nullptr, &lastFooterSize);

        // Board cells
        nvgTranslate (args.vg, BoardMargin, BoardMargin);

        auto gridder = BoardCoords (this, lastFooterSize);
        auto halfCellSize = gridder.cellSize.div (2);
        auto triggerCircleRadius = std::min (halfCellSize.x, halfCellSize.y) * .75f;

        auto modeSelect = module != nullptr ? module->currentMode : AutomataMode::Play;
        auto editTriggerMode = modeSelect >= AutomataMode::EditTrigger && modeSelect <= AutomataMode::EditTrigger_LAST;

        const auto& board = internalBoard;
        auto liveFlag = board.getLiveFlag ();
        auto triggerFlag = modeToCellTrigger (modeSelect);

        auto litColor = rack::color::WHITE;
        auto dimColor = nvgRGB (128, 128, 128);

        for (int y = 0; y < BoardHeight; y++) {
            for (int x = 0; x < BoardWidth; x++) {
                auto cellPos = gridder.getCellPos (x, y);
                nvgBeginPath (args.vg);
                nvgRect (args.vg, VEC_ARGS (cellPos), VEC_ARGS (gridder.cellSize));

                const auto& cell = board.at (x, y);

                // Get color
                auto cellLit = false;
                auto cellDim = false;
                auto hasTrigger = false;
                if (modeSelect == AutomataMode::Play)
                    cellLit = testCellFlag (cell, liveFlag);
                else if (modeSelect == AutomataMode::EditSeed) {
                    cellDim = testCellFlag (cell, liveFlag);
                    cellLit = testCellFlag (cell, AutomataCell::FLAG_SeedSet);
                    if (editing && editGrid.at (x, y))
                        cellLit = editSet;
                } else if (editTriggerMode) {
                    cellDim = testCellFlag (cell, liveFlag);
                    hasTrigger = testCellFlag (cell, triggerFlag);
                    if (editing && editGrid.at (x, y))
                        hasTrigger = editSet;
                }

                auto color = nvgRGB (64, 64, 64);
                if (cellLit)
                    color = litColor;
                else if (cellDim)
                    color = dimColor;

                nvgFillColor (args.vg, color);
                nvgFill (args.vg);

                if (hasTrigger) {
                    auto circlePos = cellPos.plus (halfCellSize);

                    nvgBeginPath (args.vg);
                    nvgCircle (args.vg, VEC_ARGS (circlePos), triggerCircleRadius);

                    nvgFillColor (args.vg, litColor);
                    nvgFill (args.vg);
                }
            }
        }

        nvgRestore (args.vg);
    }

    void AutomataBoardWidget::enterEditMode (AutomataCell mask, int cellX, int cellY, bool set) {
        assert (!editing);
        if (editing)
            return;

        editSet = set;
        editMask = mask;
        editGrid.clear ();

        if (cellX >= 0 && cellY >= 0)
            editGrid.at (cellX, cellY) = true;

        editing = true;
    }
    void AutomataBoardWidget::editModeMouseMoved (int cellX, int cellY) {
        assert (editing);

        if (!editing || cellX < 0 || cellY < 0)
            return;

        editGrid.at (cellX, cellY) = true;
    }
    void AutomataBoardWidget::exitEditMode () {
        assert (editing);
        assert (panelWidget->getAutomata () != nullptr);
        auto module = panelWidget->getAutomata ();
        if (!editing || module == nullptr)
            return;

        auto cmd = module->addEditCommand<EditCommand_Toggle> (module->lifeBoard.getBoard (), editMask, editGrid, editSet);
        cmd->execute (internalBoard);
        editCommands.push_back (cmd);

        editing = false;
    }

    void AutomataBoardWidget::onButton (const rack::event::Button& e) {
        _ThemedWidgetBase::onButton (e);

        if (e.isConsumed () || panelWidget->getRulesWidget ()->isOpen ())
            return;

        // Only allow the left mouse button with no modifier keys held.
        if (e.button != GLFW_MOUSE_BUTTON_LEFT || (e.mods & RACK_MOD_MASK) != 0)
            return;

        if (e.action == GLFW_RELEASE && editing) {
            exitEditMode ();
            e.consume (this);
            return;
        } else if (e.action != GLFW_PRESS || editing)
            return;

        auto module = panelWidget->getAutomata ();
        auto modeSelect = module->currentMode;
        if (modeSelect == AutomataMode::Play)
            return;

        AutomataCell cellMask;
        if (modeSelect == AutomataMode::EditSeed)
            cellMask = AutomataCell::FLAG_SeedSet;
        else if (modeSelect >= AutomataMode::EditTrigger && modeSelect <= AutomataMode::EditTrigger_LAST)
            cellMask = modeToCellTrigger (modeSelect);
        else
            return;

        auto gridder = BoardCoords (this, lastFooterSize);
        int cellX, cellY;
        if (!gridder.posToCell (e.pos, cellX, cellY))
            return;

        auto curSet = testCellFlag (module->lifeBoard.getBoard ().at (cellX, cellY), cellMask);
        enterEditMode (cellMask, cellX, cellY, !curSet);
        e.consume (this);
    }

    void AutomataBoardWidget::onDragHover (const rack::event::DragHover& e) {
        _ThemedWidgetBase::onDragHover (e);

        if (e.isConsumed () || panelWidget->getRulesWidget ()->isOpen ())
            return;

        if (editing && e.origin == this) {
            auto gridder = BoardCoords (this, lastFooterSize);
            int cellX, cellY;
            if (gridder.posToCell (e.pos, cellX, cellY))
                editModeMouseMoved (cellX, cellY);
            e.consume (this);
        }
    }

    void AutomataBoardWidget::onDragEnd (const rack::event::DragEnd& e) {
        _ThemedWidgetBase::onDragEnd (e);

        if (e.isConsumed () || panelWidget->getRulesWidget ()->isOpen ())
            return;

        if (e.button == GLFW_MOUSE_BUTTON_LEFT && editing) {
            exitEditMode ();

            e.consume (this);
        }
    }
}