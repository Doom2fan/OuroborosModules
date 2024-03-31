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

#include "TextField.hpp"

namespace OuroborosModules {
namespace UI {
    struct TextFieldCopyItem : rack::ui::MenuItem {
        rack::WeakPtr<TextField> textField;

        void onAction (const rack::event::Action& e) override {
            if (textField == nullptr)
                return;

            textField->copyClipboard ();
            APP->event->setSelectedWidget (textField);
        }
    };


    struct TextFieldCutItem : rack::ui::MenuItem {
        rack::WeakPtr<TextField> textField;

        void onAction (const rack::event::Action& e) override {
            if (textField == nullptr)
                return;

            textField->cutClipboard ();
            APP->event->setSelectedWidget (textField);
        }
    };


    struct TextFieldPasteItem : rack::ui::MenuItem {
        rack::WeakPtr<TextField> textField;

        void onAction (const rack::event::Action& e) override {
            if (textField == nullptr)
                return;

            textField->pasteClipboard ();
            APP->event->setSelectedWidget (textField);
        }
    };


    struct TextFieldSelectAllItem : rack::ui::MenuItem {
        rack::WeakPtr<TextField> textField;

        void onAction (const rack::event::Action& e) override {
            if (textField == nullptr)
                return;

            textField->selectAll ();
            APP->event->setSelectedWidget (textField);
        }
    };


    TextField::TextField () {
        box.size.y = BND_WIDGET_HEIGHT;
    }

    void TextField::draw (const DrawArgs& args) {
        nvgScissor (args.vg, RECT_ARGS (args.clipBox));

        BNDwidgetState state;
        if (this == APP->event->selectedWidget)
            state = BND_ACTIVE;
        else if (this == APP->event->hoveredWidget)
            state = BND_HOVER;
        else
            state = BND_DEFAULT;

        auto begin = std::min (cursor, selection);
        auto end = std::max (cursor, selection);

        auto drawText = getDisplayText ();
        bndTextField (args.vg, 0.0, 0.0, VEC_ARGS (box.size), BND_CORNER_NONE, state, -1, drawText.c_str (), begin, end);

        // Draw the placeholder text.
        if (drawText.empty ()) {
            bndIconLabelCaret (
                args.vg,
                0.0, 0.0,
                VEC_ARGS (box.size),
                -1,
                bndGetTheme ()->textFieldTheme.itemColor,
                13,
                placeholder.c_str (),
                bndGetTheme ()->textFieldTheme.itemColor,
                0, -1
            );
        }

        nvgResetScissor (args.vg);
    }

    void TextField::onDragHover (const rack::event::DragHover& e) {
        OpaqueWidget::onDragHover (e);

        if (e.origin == this)
            cursor = getTextPosition (e.pos);
    }

    void TextField::onButton (const rack::event::Button& e) {
        OpaqueWidget::onButton (e);

        if (e.action == GLFW_PRESS && e.button == GLFW_MOUSE_BUTTON_LEFT)
            cursor = selection = getTextPosition (e.pos);

        if (e.action == GLFW_PRESS && e.button == GLFW_MOUSE_BUTTON_RIGHT) {
            createContextMenu ();
            e.consume (this);
        }
    }

    void TextField::onSelectText (const rack::event::SelectText& e) {
        if (e.codepoint < 128)
            insertText (std::string (1, static_cast<char> (e.codepoint)));

        e.consume (this);
    }

    void TextField::onSelectKey (const rack::event::SelectKey& e) {
        auto maskedMods = e.mods & RACK_MOD_MASK;

        if (e.action == GLFW_PRESS || e.action == GLFW_REPEAT) {
            // Backspace
            if (e.key == GLFW_KEY_BACKSPACE && maskedMods == 0) {
                if (cursor == selection)
                    cursor = std::max (cursor - 1, 0);

                insertText ("");
                e.consume (this);
            }

            // Ctrl+Backspace
            if (e.key == GLFW_KEY_BACKSPACE && maskedMods == RACK_MOD_CTRL) {
                if (cursor == selection)
                    cursorToPrevWord ();

                insertText ("");
                e.consume (this);
            }

            // Delete
            if (e.key == GLFW_KEY_DELETE && maskedMods == 0) {
                if (cursor == selection)
                    cursor = std::min (cursor + 1, static_cast<int> (text.size ()));

                insertText ("");
                e.consume (this);
            }

            // Ctrl+Delete
            if (e.key == GLFW_KEY_DELETE && maskedMods == RACK_MOD_CTRL) {
                if (cursor == selection)
                    cursorToNextWord ();

                insertText ("");
                e.consume (this);
            }

            // Left
            if (e.key == GLFW_KEY_LEFT) {
                if (maskedMods == RACK_MOD_CTRL)
                    cursorToPrevWord ();
                else
                    cursor = std::max (cursor - 1, 0);

                if (!(e.mods & GLFW_MOD_SHIFT))
                    selection = cursor;

                e.consume (this);
            }

            // Right
            if (e.key == GLFW_KEY_RIGHT) {
                if (maskedMods == RACK_MOD_CTRL)
                    cursorToNextWord ();
                else
                    cursor = std::min (cursor + 1, static_cast<int> (text.size ()));

                if (!(e.mods & GLFW_MOD_SHIFT))
                    selection = cursor;

                e.consume (this);
            }

            // Up (placeholder)
            if (e.key == GLFW_KEY_UP)
                e.consume (this);

            // Down (placeholder)
            if (e.key == GLFW_KEY_DOWN)
                e.consume (this);

            // Home
            if (e.key == GLFW_KEY_HOME && maskedMods == 0) {
                selection = cursor = 0;
                e.consume (this);
            }

            // Shift+Home
            if (e.key == GLFW_KEY_HOME && maskedMods == GLFW_MOD_SHIFT) {
                cursor = 0;
                e.consume (this);
            }

            // End
            if (e.key == GLFW_KEY_END && maskedMods == 0) {
                selection = cursor = text.size ();
                e.consume (this);
            }

            // Shift+End
            if (e.key == GLFW_KEY_END && maskedMods == GLFW_MOD_SHIFT) {
                cursor = text.size ();
                e.consume (this);
            }

            // Ctrl+V
            if (e.keyName == "v" && maskedMods == RACK_MOD_CTRL) {
                pasteClipboard ();
                e.consume (this);
            }

            // Ctrl+X
            if (e.keyName == "x" && maskedMods == RACK_MOD_CTRL) {
                cutClipboard ();
                e.consume (this);
            }

            // Ctrl+C
            if (e.keyName == "c" && maskedMods == RACK_MOD_CTRL) {
                copyClipboard ();
                e.consume (this);
            }

            // Ctrl+A
            if (e.keyName == "a" && maskedMods == RACK_MOD_CTRL) {
                selectAll ();
                e.consume (this);
            }

            // Enter
            if ((e.key == GLFW_KEY_ENTER || e.key == GLFW_KEY_KP_ENTER) && maskedMods == 0) {
                if (multiline)
                    insertText ("\n");
                else {
                    rack::event::Action eAction;
                    onAction (eAction);
                }

                e.consume (this);
            }

            // Tab
            if (e.key == GLFW_KEY_TAB && maskedMods == 0) {
                if (nextField)
                    APP->event->setSelectedWidget (nextField);

                e.consume (this);
            }

            // Shift+Tab
            if (e.key == GLFW_KEY_TAB && maskedMods == GLFW_MOD_SHIFT) {
                if (prevField)
                    APP->event->setSelectedWidget (prevField);

                e.consume (this);
            }

            // Consume all printable keys unless Ctrl is held
            if ((e.mods & RACK_MOD_CTRL) == 0 && !e.keyName.empty ())
                e.consume (this);

            assert (0 <= cursor);
            assert (cursor <= static_cast<int> (text.size ()));
            assert (0 <= selection);
            assert (selection <= static_cast<int> (text.size ()));
        }
    }

    int TextField::getTextPosition (rack::math::Vec mousePos) {
        return bndTextFieldTextPosition (
            APP->window->vg,
            0.0, 0.0,
            VEC_ARGS (box.size),
            -1,
            text.c_str (),
            mousePos.x, mousePos.y
        );
    }

    void TextField::setMaxLength (std::string::size_type newLength) {
        maxLength = newLength;
    }

    std::string TextField::getText () { return text; }

    void TextField::setText (std::string text) {
        if (this->text != text) {
            this->text = text;

            rack::event::Change eChange;
            onChange (eChange);
        }

        selection = cursor = text.size ();
    }

    std::string TextField::getDisplayText () {
        return text;
    }

    void TextField::selectAll () {
        cursor = text.size ();
        selection = 0;
    }

    std::string TextField::getSelectedText () {
        auto begin = std::min (cursor, selection);
        auto len = std::abs (selection - cursor);
        return text.substr (begin, len);
    }

    void TextField::insertText (std::string newText) {
        auto changed = false;

        if (cursor != selection) {
            // Delete selected text
            auto begin = std::min (cursor, selection);
            auto len = std::abs (selection - cursor);
            text.erase (begin, len);
            cursor = selection = begin;
            changed = true;
        }

        if (!newText.empty ()) {
            auto spareLength = maxLength - text.length ();
            if (newText.length () > spareLength)
                newText.erase (newText.begin () + spareLength, newText.end ());
        }

        if (!newText.empty ()) {
            text.insert (cursor, newText);
            cursor += newText.size ();
            selection = cursor;
            changed = true;
        }

        if (changed) {
            rack::event::Change eChange;
            onChange (eChange);
        }
    }

    void TextField::copyClipboard () {
        if (cursor == selection)
            return;

        glfwSetClipboardString (APP->window->win, getSelectedText ().c_str ());
    }

    void TextField::cutClipboard () {
        copyClipboard ();
        insertText ("");
    }

    void TextField::pasteClipboard () {
        auto newText = glfwGetClipboardString (APP->window->win);
        if (!newText)
            return;

        insertText (newText);
    }

    void TextField::cursorToPrevWord () {
        auto pos = text.rfind (' ', std::max (cursor - 2, 0));

        if (pos == std::string::npos)
            cursor = 0;
        else
            cursor = std::min (static_cast<int> (pos) + 1, static_cast<int> (text.size ()));
    }

    void TextField::cursorToNextWord () {
        auto pos = text.find (' ', std::min (cursor + 1, static_cast<int> (text.size ())));
        if (pos == std::string::npos)
            pos = text.size ();

        cursor = pos;
    }

    void TextField::createContextMenu () {
        auto menu = rack::createMenu ();

        auto cutItem = new TextFieldCutItem;
        cutItem->text = "Cut";
        cutItem->rightText = RACK_MOD_CTRL_NAME "+X";
        cutItem->textField = this;
        menu->addChild (cutItem);

        auto copyItem = new TextFieldCopyItem;
        copyItem->text = "Copy";
        copyItem->rightText = RACK_MOD_CTRL_NAME "+C";
        copyItem->textField = this;
        menu->addChild (copyItem);

        auto pasteItem = new TextFieldPasteItem;
        pasteItem->text = "Paste";
        pasteItem->rightText = RACK_MOD_CTRL_NAME "+V";
        pasteItem->textField = this;
        menu->addChild (pasteItem);

        auto selectAllItem = new TextFieldSelectAllItem;
        selectAllItem->text = "Select all";
        selectAllItem->rightText = RACK_MOD_CTRL_NAME "+A";
        selectAllItem->textField = this;
        menu->addChild (selectAllItem);
    }
}
}