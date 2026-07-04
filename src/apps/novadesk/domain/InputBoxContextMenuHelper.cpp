/* Copyright (C) 2026 OfficialNovadesk
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "InputBoxContextMenuHelper.h"

#include <windows.h>
#include <string>
#include <vector>

#include "Widget.h"
#include "../render/InputBoxElement.h"
#include "../scripting/quickjs/engine/JSEngine.h"
#include "../shared/System.h"
#include "../shared/MenuUtils.h"

namespace
{
    constexpr int CMD_UNDO      = 50001;
    constexpr int CMD_REDO      = 50002;
    constexpr int CMD_CUT       = 50003;
    constexpr int CMD_COPY      = 50004;
    constexpr int CMD_PASTE     = 50005;
    constexpr int CMD_DELETE    = 50006;
    constexpr int CMD_SELECTALL = 50007;
}

namespace InputBoxContextMenuHelper
{

bool ShowInputBoxContextMenu(Widget &widget, InputBoxElement *inputElem,
                              int clientX, int clientY)
{
    if (!inputElem)
        return false;

    HWND hWnd = widget.GetWindow();
    bool needRedraw = false;

    // Focus management
    InputBoxElement *prevFocused = widget.GetFocusedInputBox();
    if (prevFocused && prevFocused != inputElem)
    {
        if (prevFocused->m_OnBlurCallbackId != -1)
            JSEngine::CallEventCallback(prevFocused->m_OnBlurCallbackId, &widget, nullptr);
        prevFocused->SetFocus(false);
        KillTimer(hWnd, 5 /*TIMER_CARET*/);
    }
    if (!inputElem->IsFocused())
    {
        inputElem->SetFocus(true);
        SetTimer(hWnd, 5 /*TIMER_CARET*/, 530, nullptr);
        if (inputElem->m_OnFocusCallbackId != -1)
            JSEngine::CallEventCallback(inputElem->m_OnFocusCallbackId, &widget, nullptr);
    }
    widget.SetFocusedInputBox(inputElem);

    // Only move the caret to the click point when there is no active
    // selection — preserves Ctrl+A or mouse-drag selections.
    if (!inputElem->HasSelection())
    {
        bool shift = (GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0;
        inputElem->HandleMouseDown(clientX, clientY, shift);
        inputElem->HandleMouseUp();
    }
    widget.SetFocus();
    needRedraw = true;

    // Build menu
    bool hasSelection = inputElem->HasSelection();
    bool isPassword   = inputElem->IsPasswordMode();

    std::wstring clipText;
    bool canPaste = novadesk::shared::system::ClipboardGetText(clipText) && !clipText.empty();

    std::vector<MenuItem> menuItems;
    menuItems.push_back({ L"&Undo", CMD_UNDO, false, false, !inputElem->CanUndo() });
    menuItems.push_back({ L"&Redo", CMD_REDO, false, false, !inputElem->CanRedo() });
    
    MenuItem sep;
    sep.isSeparator = true;
    menuItems.push_back(sep);

    menuItems.push_back({ L"Cu&t", CMD_CUT, false, false, !(hasSelection && !isPassword) });
    menuItems.push_back({ L"&Copy", CMD_COPY, false, false, !(hasSelection && !isPassword) });
    menuItems.push_back({ L"&Paste", CMD_PASTE, false, false, !canPaste });
    menuItems.push_back({ L"&Delete", CMD_DELETE, false, false, !hasSelection });
    
    menuItems.push_back(sep);
    
    menuItems.push_back({ L"Select &All", CMD_SELECTALL, false, false, inputElem->GetText().empty() });

    HMENU hMenu = CreatePopupMenu();
    MenuUtils::BuildMenu(hMenu, menuItems);

    POINT screenPt = { clientX, clientY };
    ClientToScreen(hWnd, &screenPt);

    SetForegroundWindow(hWnd);
    const int cmd = TrackPopupMenu(hMenu, TPM_RETURNCMD | TPM_NONOTIFY | TPM_BOTTOMALIGN | TPM_LEFTALIGN,
                                   screenPt.x, screenPt.y, 0, hWnd, NULL);
    DestroyMenu(hMenu);

    // Execute command─
    if (cmd != 0)
    {
        bool textChanged = false;

        if (cmd == CMD_UNDO)
        {
            textChanged = inputElem->Undo();
        }
        else if (cmd == CMD_REDO)
        {
            textChanged = inputElem->Redo();
        }
        else if (cmd == CMD_CUT)
        {
            std::wstring sel = inputElem->GetSelectedText();
            if (!sel.empty())
            {
                novadesk::shared::system::ClipboardSetText(sel);
                inputElem->DeleteSelection();
                textChanged = true;
            }
        }
        else if (cmd == CMD_COPY)
        {
            std::wstring sel = inputElem->GetSelectedText();
            if (!sel.empty())
            {
                novadesk::shared::system::ClipboardSetText(sel);
            }
        }
        else if (cmd == CMD_PASTE)
        {
            if (canPaste)
            {
                inputElem->ReplaceSelection(clipText);
                textChanged = true;
            }
        }
        else if (cmd == CMD_DELETE)
        {
            if (hasSelection)
            {
                inputElem->DeleteSelection();
                textChanged = true;
            }
        }
        else if (cmd == CMD_SELECTALL)
        {
            inputElem->SelectAll();
        }

        if (textChanged && inputElem->m_OnTextChangeCallbackId != -1)
        {
            JSEngine::CallEventCallbackWithText(inputElem->m_OnTextChangeCallbackId,
                                                &widget, inputElem->GetText());
        }

        needRedraw = true;
    }

    return needRedraw;
}

} // namespace InputBoxContextMenuHelper
