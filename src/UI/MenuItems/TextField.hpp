/*
 *  OuroborosModules
 *  Copyright (C) 2024 Chronos "phantombeta" Ouroboros
 *  Copyright (C) 2016-2023 VCV
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

#include "../../PluginDef.hpp"

#include <limits>

namespace OuroborosModules {
namespace UI {
    struct TextField : rack::widget::OpaqueWidget {
        std::string text;
        std::string placeholder;

        /** The maximum amount of characters the text field is allowed to hold. */
        std::string::size_type maxLength = std::numeric_limits<std::string::size_type>::max ();
        bool multiline = false;
        /** The index of the text cursor */
        int cursor = 0;
        /** The index of the other end of the selection. If nothing is selected, this is equal to `cursor`. */
        int selection = 0;

        /** For Tab and Shift-Tab focusing. */
        Widget* prevField = NULL;
        Widget* nextField = NULL;

        TextField ();

        void draw (const DrawArgs& args) override;
        void onDragHover (const DragHoverEvent& e) override;
        void onButton (const ButtonEvent& e) override;
        void onSelectText (const SelectTextEvent& e) override;
        void onSelectKey (const SelectKeyEvent& e) override;

        virtual int getTextPosition (rack::math::Vec mousePos);

        virtual void setMaxLength (std::string::size_type newLength);

        std::string getText ();
        /** Replaces the entire text */
        virtual void setText (std::string text);
        /** Gets the the text to be displayed in the field. Override to modify how the text is displayed. */
        virtual std::string getDisplayText ();

        void selectAll ();
        std::string getSelectedText ();

        /** Inserts text at the cursor, replacing the selection if necessary */
        virtual void insertText (std::string newText);

        void copyClipboard ();
        void cutClipboard ();
        void pasteClipboard ();

        void cursorToPrevWord ();
        void cursorToNextWord ();

        virtual void createContextMenu ();
    };
}
}