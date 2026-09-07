// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Vixen420
//
// Subgroup is free software: you may redistribute it and/or modify it under the
// terms of the GNU General Public License as published by the Free Software
// Foundation, either version 3 of the License, or (at your option) any later
// version. It comes with ABSOLUTELY NO WARRANTY. See the file LICENSE, or
// <https://www.gnu.org/licenses/>, for the full text.

/*
 * Group — the augmented Ctrl+G.
 *
 * Assign Ctrl+G to this script (Edit -> Keyboard Shortcuts -> Menu Commands ->
 * File -> Scripts -> Subgroup -> Group) and grouping starts working in the
 * cases Illustrator declines, with no change to any case it already handles.
 *
 * Design rules, because this stands in for a reflex keystroke:
 *
 *   1. Illustrator gets first refusal. Whenever the native command acts, the
 *      result IS vanilla — not an imitation of it. We only take over after
 *      watching it decline.
 *   2. Never raise a dialog. Vanilla fails silently when it cannot group.
 *   3. Never throw. A keystroke that errors is worse than one that does
 *      nothing, so the whole augmented path is guarded.
 *   4. Toggle off -> literally just the native command, nothing else.
 */

#include "lib/SubgroupCore.jsxinc"
#include "lib/SubgroupPrefs.jsxinc"

(function () {
    SG_QUIET = true;

    function vanilla() {
        try { app.executeMenuCommand("group"); } catch (e) {}
    }

    if (app.documents.length === 0) { vanilla(); return; }

    /* Rule 4: switched off means switched off. */
    if (!sgPrefBool(SG_PREF_NESTING, true)) { vanilla(); return; }

    var sel = sgSelectionArray();
    if (sel.length === 0) { vanilla(); return; }

    /* Rule 1: let Illustrator try, and detect whether it actually did anything.
       If it grouped, the probe item has been reparented into the new group. */
    var probe = sel[0];
    var before;
    try { before = probe.parent; } catch (e) { vanilla(); return; }

    vanilla();

    try {
        if (probe.parent !== before) return;   /* native handled it */
    } catch (e) {
        return;                                /* item is gone; leave well alone */
    }

    /* Illustrator declined. Do what it would have done had the selection been
       representable in the first place. */
    try {
        if (sel.length === 1 && sgIsGroup(sel[0])) {
            var made;
            if (sgPrefStr(SG_PREF_GROUPMODE, "nest") === "wrap") {
                made = sgWrap(sel);
            } else {
                /* Faithful to the junk-object workaround: selecting the
                   children and grouping them puts the new level INSIDE. */
                made = sgNestContents(sel[0]);
            }
            if (made) app.selection = [made];
            return;
        }

        if (sgCheckAll(sel)) return;   /* locked or hidden: stay silent, as vanilla does */

        var grp = sgWrap(sel);
        if (grp) app.selection = [grp];
    } catch (e) {
        /* Rule 3. */
    }
})();
