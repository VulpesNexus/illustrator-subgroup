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

/* The About dialog.
 *
 * Built by hand rather than through the SDK's SDKAboutPluginsHelper, and rather
 * than through Windows' task dialog, because both refuse the two things this
 * box is supposed to do.
 *
 *   - AIUserSuite::MessageAlert, which the SDK helper ends in, is a plain OS
 *     alert: one run of text, no emphasis, no links. It also takes a char* and
 *     decodes it in the *platform* encoding, so an em dash or a copyright sign
 *     turns to mojibake anywhere the code page is not Latin-1.
 *   - The task dialog can hold links, but only in its content and footer. Its
 *     main instruction - the one piece of text with any visual weight - cannot
 *     be one, and nothing in it can be emphasized.
 *
 * So: a dialog resource with SysLink controls where a link is wanted and a bold
 * font where weight is wanted. The command names are bold rather than
 * underlined on purpose. Underline reads as "clickable" to everyone who has
 * used a computer, and these are not; bold brings them forward without making
 * that promise, and without the eyesore of a second heading-sized run.
 *
 * There are no Illustrator types below. That is what lets tools/AboutHarness
 * build this same dialog into a standalone executable and show it, which is the
 * only way to actually look at a modal dialog that otherwise only ever appears
 * inside a host application.
 */

#ifdef _WIN32

#include "SubgroupAbout.h"
#include "SubgroupID.h"
#include "Resource.h"

#include <commctrl.h>
#include <shellapi.h>

#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "comctl32.lib")

/* SG_WVERSION, SG_EMDASH, SG_COPY and the two URLs come from SubgroupID.h, so
   the dialog and the plain-alert fallback in SubgroupPlugin.cpp cannot drift
   apart. */

namespace {

struct AboutFonts {
    HFONT title;
    HFONT bold;
    HFONT small_;
};

/* Derives a font from the dialog's own, so it follows whatever the user's shell
   font and DPI actually are instead of hard-coding a face and a pixel size. */
HFONT DeriveFont(HWND dlg, int percentOfHeight, bool bold)
{
    HFONT base = reinterpret_cast<HFONT>(SendMessageW(dlg, WM_GETFONT, 0, 0));
    if (base == nullptr) return nullptr;

    LOGFONTW lf;
    if (GetObjectW(base, sizeof(lf), &lf) == 0) return nullptr;

    /* lfHeight is negative for character height, so scale the magnitude. */
    LONG h = lf.lfHeight;
    lf.lfHeight = (h < 0) ? -MulDiv(-h, percentOfHeight, 100)
                          :  MulDiv( h, percentOfHeight, 100);
    if (bold) lf.lfWeight = FW_BOLD;

    return CreateFontIndirectW(&lf);
}

void SetControlFont(HWND dlg, int id, HFONT font)
{
    if (font == nullptr) return;
    HWND ctl = GetDlgItem(dlg, id);
    if (ctl != nullptr) SendMessageW(ctl, WM_SETFONT, reinterpret_cast<WPARAM>(font), TRUE);
}

/* Opens a link. Anything a SysLink hands back is one of ours, from the resource
   below, but check the scheme anyway: ShellExecute will happily launch things
   that are not URLs, and a habit of feeding it unchecked strings is how that
   turns into a bug later. */
void OpenLink(HWND parent, const wchar_t* url)
{
    if (url == nullptr) return;
    if (wcsncmp(url, L"https://", 8) != 0) return;
    ShellExecuteW(parent, L"open", url, nullptr, nullptr, SW_SHOWNORMAL);
}

INT_PTR CALLBACK AboutProc(HWND dlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg) {
    case WM_INITDIALOG: {
        AboutFonts* fonts = new AboutFonts();
        fonts->title  = DeriveFont(dlg, 150, true);
        fonts->bold   = DeriveFont(dlg, 100, true);
        fonts->small_ = DeriveFont(dlg,  92, false);
        SetWindowLongPtrW(dlg, DWLP_USER, reinterpret_cast<LONG_PTR>(fonts));

        SetControlFont(dlg, IDC_ABOUT_TITLE, fonts->title);
        SetControlFont(dlg, IDC_ABOUT_NAME1, fonts->bold);
        SetControlFont(dlg, IDC_ABOUT_NAME2, fonts->bold);
        SetControlFont(dlg, IDC_ABOUT_NAME3, fonts->bold);
        SetControlFont(dlg, IDC_ABOUT_LEGAL, fonts->small_);

        SetDlgItemTextW(dlg, IDC_ABOUT_TITLE,
            L"<a href=\"" SG_REPO_URL L"\">Subgroup " SG_WVERSION L"</a>");

        SetDlgItemTextW(dlg, IDC_ABOUT_NAME1, L"Nest Down");
        SetDlgItemTextW(dlg, IDC_ABOUT_DESC1,
            L"Adds a level of grouping inside the selection, including when the "
            L"selection is an entire group " SG_EMDASH L" the case Object > Group "
            L"declines.");

        SetDlgItemTextW(dlg, IDC_ABOUT_NAME2, L"Nest Up");
        SetDlgItemTextW(dlg, IDC_ABOUT_DESC2,
            L"Adds that level around the selection instead.");

        SetDlgItemTextW(dlg, IDC_ABOUT_NAME3, L"Align Group To Selected");
        SetDlgItemTextW(dlg, IDC_ABOUT_DESC3,
            L"Aligns a group's contents to one object selected inside it, which "
            L"Illustrator's own Align cannot reach.");

        /* The GPL asks an interactive program to show this where the user can
           find it. For a plug-in with no window of its own, that is here. */
        SetDlgItemTextW(dlg, IDC_ABOUT_LEGAL,
            L"Copyright " SG_COPY L" 2026 <a href=\"" SG_AUTHOR_URL L"\">Vixen420</a>. "
            L"Free software under the GNU General Public License, version 3 or "
            L"later, with an Adobe Illustrator SDK linking exception. It comes "
            L"with ABSOLUTELY NO WARRANTY.");

        /* Center on the owner, or on the screen when there is none. */
        RECT dr;
        GetWindowRect(dlg, &dr);
        HWND owner = GetWindow(dlg, GW_OWNER);
        RECT pr;
        if (owner != nullptr && IsWindowVisible(owner)) GetWindowRect(owner, &pr);
        else SystemParametersInfoW(SPI_GETWORKAREA, 0, &pr, 0);
        SetWindowPos(dlg, nullptr,
                     pr.left + ((pr.right - pr.left) - (dr.right - dr.left)) / 2,
                     pr.top  + ((pr.bottom - pr.top) - (dr.bottom - dr.top)) / 2,
                     0, 0, SWP_NOSIZE | SWP_NOZORDER);

        /* Focus OK rather than letting the dialog manager give it to the first
           tab stop, which is the title link: it would open with a focus
           rectangle drawn around the heading, and Enter would open a browser
           instead of closing the box. Returning FALSE says focus is set. */
        SetFocus(GetDlgItem(dlg, IDOK));
        return FALSE;
    }

    case WM_NOTIFY: {
        const NMHDR* hdr = reinterpret_cast<const NMHDR*>(lParam);
        if (hdr != nullptr && (hdr->code == NM_CLICK || hdr->code == NM_RETURN) &&
            (hdr->idFrom == IDC_ABOUT_TITLE || hdr->idFrom == IDC_ABOUT_LEGAL)) {
            const NMLINK* link = reinterpret_cast<const NMLINK*>(lParam);
            OpenLink(dlg, link->item.szUrl);
            return TRUE;
        }
        break;
    }

    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL) {
            EndDialog(dlg, LOWORD(wParam));
            return TRUE;
        }
        break;

    case WM_DESTROY: {
        AboutFonts* fonts =
            reinterpret_cast<AboutFonts*>(GetWindowLongPtrW(dlg, DWLP_USER));
        if (fonts != nullptr) {
            if (fonts->title)  DeleteObject(fonts->title);
            if (fonts->bold)   DeleteObject(fonts->bold);
            if (fonts->small_) DeleteObject(fonts->small_);
            delete fonts;
            SetWindowLongPtrW(dlg, DWLP_USER, 0);
        }
        break;
    }

    default:
        break;
    }
    return FALSE;
}

} /* anonymous namespace */

bool SubgroupShowAboutDialog(HINSTANCE instance, HWND parent)
{
    /* SysLink lives in version 6 of the common controls. Whether a host process
       has that in its activation context is the host's business, so ask rather
       than assume - and let the caller fall back if the answer is no. */
    INITCOMMONCONTROLSEX icc;
    icc.dwSize = sizeof(icc);
    icc.dwICC  = ICC_LINK_CLASS | ICC_STANDARD_CLASSES;
    if (!InitCommonControlsEx(&icc)) return false;

    INT_PTR r = DialogBoxParamW(instance, MAKEINTRESOURCEW(IDD_SUBGROUP_ABOUT),
                                parent, AboutProc, 0);
    return r != -1 && r != 0;
}

#endif /* _WIN32 */
