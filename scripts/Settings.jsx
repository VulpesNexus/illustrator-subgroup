// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Vixen420
//
// Subgroup is free software: you may redistribute it and/or modify it under the
// terms of the GNU General Public License as published by the Free Software
// Foundation, either version 3 of the License, or (at your option) any later
// version. It comes with ABSOLUTELY NO WARRANTY. See the file LICENSE, or
// <https://www.gnu.org/licenses/>, for the full text.

/*
 * Settings — the toggles. Written to app.preferences, so they survive restarts.
 */

#include "lib/SubgroupCore.jsxinc"
#include "lib/SubgroupPrefs.jsxinc"

(function () {
    var dlg = new Window("dialog", "Subgroup Settings");
    dlg.orientation = "column";
    dlg.alignChildren = "fill";
    dlg.spacing = 12;
    dlg.margins = 16;

    var p1 = dlg.add("panel", undefined, "Augmented grouping");
    p1.orientation = "column";
    p1.alignChildren = "left";
    p1.margins = 14;
    p1.spacing = 8;

    var on = p1.add("checkbox", undefined,
                    "Let Ctrl+G group a selection that is a whole group");
    on.value = sgPrefBool(SG_PREF_NESTING, true);

    var note = p1.add("statictext", undefined,
        "Off, the Group script does nothing but call Illustrator's own command.\n" +
        "On, it still calls it first and only steps in when Illustrator declines,\n" +
        "so every case that already works keeps working exactly as before.",
        { multiline: true });
    note.preferredSize = [400, 46];

    var p2 = dlg.add("panel", undefined, "When the selection is an entire group, add the level");
    p2.orientation = "column";
    p2.alignChildren = "left";
    p2.margins = 14;
    p2.spacing = 6;

    var rNest = p2.add("radiobutton", undefined,
                       "Inside it  —  Group(A, B) → Group(Group(A, B))");
    var rWrap = p2.add("radiobutton", undefined,
                       "Around it  —  Group(A, B) → Group(Group(A, B)) as a new outer group");
    var mode = sgPrefStr(SG_PREF_GROUPMODE, "nest");
    rNest.value = (mode !== "wrap");
    rWrap.value = (mode === "wrap");

    var note2 = p2.add("statictext", undefined,
        "Both produce the same shape. They differ in which group object ends up\n" +
        "on the outside, which matters if the original carries an appearance.\n" +
        "Inside is what the junk-object workaround produces.",
        { multiline: true });
    note2.preferredSize = [400, 46];

    var p3 = dlg.add("panel", undefined, "Keyboard");
    p3.orientation = "column";
    p3.alignChildren = "left";
    p3.margins = 14;
    var hint = p3.add("statictext", undefined,
        "None of this is active until Ctrl+G points here:\n" +
        "Edit > Keyboard Shortcuts > Menu Commands > File > Scripts > Subgroup > Group.\n" +
        "Illustrator will warn that Ctrl+G is taken by Object > Group; accept, and\n" +
        "save the set. Clearing that assignment restores stock behavior entirely.",
        { multiline: true });
    hint.preferredSize = [400, 62];

    var foot = dlg.add("group");
    foot.alignment = "right";
    var cancel = foot.add("button", undefined, "Cancel", { name: "cancel" });
    var save = foot.add("button", undefined, "Save", { name: "ok" });

    save.onClick = function () {
        sgPrefSetBool(SG_PREF_NESTING, on.value);
        sgPrefSetStr(SG_PREF_GROUPMODE, rWrap.value ? "wrap" : "nest");
        dlg.close(1);
    };
    cancel.onClick = function () { dlg.close(0); };

    dlg.show();
})();
