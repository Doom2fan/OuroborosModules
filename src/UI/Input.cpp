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

#include "Input.hpp"

#include <fmt/format.h>

namespace OuroborosModules {
namespace Input {
    std::string keyName (int key) {
        if (key >= GLFW_KEY_F1 && key <= GLFW_KEY_F25)
            return fmt::format (FMT_STRING ("F{}"), key - GLFW_KEY_F1);
        if (key >= GLFW_KEY_KP_0 && key <= GLFW_KEY_KP_9)
            return fmt::format (FMT_STRING ("Keypad {}"), key - GLFW_KEY_KP_0);

        switch (key) {
            case GLFW_KEY_UNKNOWN: return "";

            case GLFW_KEY_SPACE:  return "Space";
            case GLFW_KEY_TAB:    return "Tab";
            case GLFW_KEY_ESCAPE: return "Esc";

            case GLFW_KEY_UP:    return "Up";
            case GLFW_KEY_DOWN:  return "Down";
            case GLFW_KEY_LEFT:  return "Left";
            case GLFW_KEY_RIGHT: return "Right";


            case GLFW_KEY_ENTER:     return "Enter";
            case GLFW_KEY_BACKSPACE: return "Backspace";

            case GLFW_KEY_INSERT:    return "Insert";
            case GLFW_KEY_DELETE:    return "Delete";
            case GLFW_KEY_PAGE_UP:   return "Page Up";
            case GLFW_KEY_PAGE_DOWN: return "Page Down";
            case GLFW_KEY_HOME:      return "Home";
            case GLFW_KEY_END:       return "End";

            case GLFW_KEY_PRINT_SCREEN: return "PrtSc";
            case GLFW_KEY_PAUSE:        return "Pause";

            case GLFW_KEY_KP_DIVIDE:   return "Keypad /";
            case GLFW_KEY_KP_MULTIPLY: return "Keypad *";
            case GLFW_KEY_KP_SUBTRACT: return "Keypad -";
            case GLFW_KEY_KP_ADD:      return "Keypad +";
            case GLFW_KEY_KP_DECIMAL:  return "Keypad .";

            default: break;
        }

        auto k = glfwGetKeyName (key, 0);
        if (k != nullptr)
            return std::string (k);

        return "";
    }

    std::string buttonName (int button) {
        switch (button) {
            case -1: return "";

            case GLFW_MOUSE_BUTTON_LEFT:   return "LMB";
            case GLFW_MOUSE_BUTTON_RIGHT:  return "RMB";
            case GLFW_MOUSE_BUTTON_MIDDLE: return "MMB";

            default: return fmt::format (FMT_STRING ("M{}"), button);
        }
    }
}
}