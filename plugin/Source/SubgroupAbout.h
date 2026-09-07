// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Vixen420
//
// Subgroup is free software: you may redistribute it and/or modify it under the
// terms of the GNU General Public License as published by the Free Software
// Foundation, either version 3 of the License, or (at your option) any later
// version. It comes with ABSOLUTELY NO WARRANTY. See the file LICENSE, or
// <https://www.gnu.org/licenses/>, for the full text.
//
// Additional permission under GPL-3.0 section 7: this file may be combined with
// the Adobe Illustrator SDK, whose sample framework sources are compiled into
// every plug-in built from it. See LICENSE-EXCEPTION.

#ifndef __SUBGROUPABOUT_H__
#define __SUBGROUPABOUT_H__

#ifdef _WIN32

#include <windows.h>

/** Shows the About dialog.

    Deliberately free of every Illustrator dependency: it takes a module handle
    and a parent window and touches nothing else. That is what lets the dialog be
    built into a small harness and looked at directly, instead of being inspected
    only through the host - which for a modal dialog means not at all.

    @param  instance  the module holding the dialog resource.
    @param  parent    owner window, may be null.
    @return false if the dialog could not be shown, so the caller can fall back
            to something plainer. */
bool SubgroupShowAboutDialog(HINSTANCE instance, HWND parent);

#endif /* _WIN32 */

#endif /* __SUBGROUPABOUT_H__ */
