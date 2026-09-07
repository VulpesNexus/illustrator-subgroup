// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Vixen420
//
// Subgroup is free software: you may redistribute it and/or modify it under the
// terms of the GNU General Public License as published by the Free Software
// Foundation, either version 3 of the License, or (at your option) any later
// version. It comes with ABSOLUTELY NO WARRANTY. See the file LICENSE, or
// <https://www.gnu.org/licenses/>, for the full text.

/* Shows the plug-in's About dialog on its own, outside Illustrator.
 *
 * A modal dialog inside a host application is close to untestable: you cannot
 * drive it, screenshot it, or measure it without a person sitting there. This
 * builds the very same SubgroupAbout.cpp and the very same dialog resource into
 * a two-hundred-line executable, so the layout can be looked at, and looked at
 * again after every edit.
 *
 * Build it with tools/AboutHarness/build.cmd. It is a development tool and is
 * not part of the plug-in.
 */

#include <windows.h>
#include "SubgroupAbout.h"

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE, LPWSTR cmdLine, int)
{
    /* /exit<ms> closes the dialog by itself, so a screenshot can be taken
       without anything hanging around waiting for a click. */
    int autoCloseMs = 0;
    if (cmdLine != nullptr && wcsncmp(cmdLine, L"/exit", 5) == 0)
        autoCloseMs = _wtoi(cmdLine + 5);

    if (autoCloseMs > 0) {
        struct Closer {
            static DWORD WINAPI Run(LPVOID ms) {
                Sleep(static_cast<DWORD>(reinterpret_cast<UINT_PTR>(ms)));
                HWND dlg = FindWindowW(nullptr, L"About Subgroup");
                if (dlg != nullptr) PostMessageW(dlg, WM_COMMAND, IDCANCEL, 0);
                return 0;
            }
        };
        CloseHandle(CreateThread(nullptr, 0, Closer::Run,
                    reinterpret_cast<LPVOID>(static_cast<UINT_PTR>(autoCloseMs)),
                    0, nullptr));
    }

    if (!SubgroupShowAboutDialog(instance, nullptr)) {
        MessageBoxW(nullptr, L"SubgroupShowAboutDialog returned false: the "
                             L"dialog could not be created.",
                    L"About harness", MB_ICONERROR | MB_OK);
        return 1;
    }
    return 0;
}
