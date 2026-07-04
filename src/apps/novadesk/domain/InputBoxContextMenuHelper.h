/* Copyright (C) 2026 OfficialNovadesk
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#pragma once

#include <windows.h>

class Widget;
class InputBoxElement;

namespace InputBoxContextMenuHelper
{
    /**
     * Show the standard edit context menu (Undo, Redo, Cut, Copy, Paste,
     * Delete, Select All) for the given InputBoxElement.
     *
     * Handles focus transfer, caret placement, clipboard operations and fires
     * the JS onTextChange callback when the text is modified.
     *
     * @param widget     The owning Widget (used for focus timers and JS callbacks).
     * @param inputElem  The InputBoxElement that was right-clicked.
     * @param clientX    Client-area x of the right-click position.
     * @param clientY    Client-area y of the right-click position.
     *
     * @return true if the widget needs a redraw after this call.
     */
    bool ShowInputBoxContextMenu(Widget &widget, InputBoxElement *inputElem,
                                 int clientX, int clientY);
}
