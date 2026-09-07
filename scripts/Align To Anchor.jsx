// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Vixen420
//
// Subgroup is free software: you may redistribute it and/or modify it under the
// terms of the GNU General Public License as published by the Free Software
// Foundation, either version 3 of the License, or (at your option) any later
// version. It comes with ABSOLUTELY NO WARRANTY. See the file LICENSE, or
// <https://www.gnu.org/licenses/>, for the full text.

/*
 * Align Inside Group — align objects to a chosen anchor, including the case
 * Illustrator's Align panel cannot reach.
 *
 * The Align panel's "key object" is designated by clicking an already
 * selected object, which needs two or more separately selected objects. When
 * a group holds exactly the objects you want to align, selecting them all
 * collapses to selecting the group, so there is never a key object to set.
 *
 * This script sidesteps the selection model entirely: select the GROUP and
 * pick the anchor from a list of its children. It also accepts an ordinary
 * multi-object selection.
 */

#include "lib/SubgroupCore.jsxinc"

(function () {
    if (!sgActiveDoc()) return;

    /* ---- work out what we are aligning ---------------------------------- */
    var sel = sgSelectionArray();
    var items, source;

    if (sel.length === 1 && sgIsGroup(sel[0])) {
        items = sgChildren(sel[0]);
        source = "children of the selected group";
    } else if (sel.length >= 2) {
        items = sel;
        source = "the selected objects";
    } else {
        sgAlert("Select a group, or two or more objects.\n\n" +
                "Selecting a group lets you align its children to one of " +
                "them — the case the Align panel cannot reach.");
        return;
    }

    if (items.length < 2) {
        sgAlert("Need at least two objects to align.\n" +
                "That group holds " + items.length + ".");
        return;
    }

    var why = sgCheckAll(items);
    if (why) { sgAlert("Cannot align:\n" + why); return; }

    var restore = sel;

    /* ---- geometry -------------------------------------------------------- */
    function boundsOf(item, preview) {
        return preview ? item.visibleBounds : item.geometricBounds;
    }

    function applyAlign(mode, anchor, preview) {
        var ab = boundsOf(anchor, preview);
        for (var i = 0; i < items.length; i++) {
            var it = items[i];
            if (it === anchor) continue;
            var b = boundsOf(it, preview);
            var dx = 0, dy = 0;
            switch (mode) {
            case "left":    dx = ab[0] - b[0]; break;
            case "hcenter": dx = (ab[0] + ab[2]) / 2 - (b[0] + b[2]) / 2; break;
            case "right":   dx = ab[2] - b[2]; break;
            case "top":     dy = ab[1] - b[1]; break;
            case "vcenter": dy = (ab[1] + ab[3]) / 2 - (b[1] + b[3]) / 2; break;
            case "bottom":  dy = ab[3] - b[3]; break;
            }
            if (dx !== 0 || dy !== 0) it.translate(dx, dy);
        }
        try { app.redraw(); } catch (e) {}
    }

    /* ---- dialog ---------------------------------------------------------- */
    var dlg = new Window("dialog", "Align Inside Group");
    dlg.orientation = "column";
    dlg.alignChildren = "fill";
    dlg.spacing = 10;
    dlg.margins = 16;

    dlg.add("statictext", undefined,
            "Anchor — everything else moves to meet it. Aligning " +
            source + ".");

    var labels = [];
    for (var i = 0; i < items.length; i++) {
        labels.push((i + 1) + ".  " + sgLabel(items[i]));
    }
    var list = dlg.add("listbox", undefined, labels);
    list.preferredSize = [340, 150];
    list.selection = 0;

    /* Selecting a row highlights that object on the canvas, which is the
       only practical way to tell two unnamed paths apart. */
    list.onChange = function () {
        if (!list.selection) return;
        try {
            app.selection = [items[list.selection.index]];
            app.redraw();
        } catch (e) {}
    };

    var prev = dlg.add("checkbox", undefined,
                       "Use preview bounds (include strokes and effects)");
    prev.value = false;

    function anchor() {
        return items[list.selection ? list.selection.index : 0];
    }

    var hp = dlg.add("panel", undefined, "Horizontal");
    hp.orientation = "row";
    hp.alignChildren = "fill";
    hp.margins = 12;
    var hL = hp.add("button", undefined, "Left");
    var hC = hp.add("button", undefined, "Center");
    var hR = hp.add("button", undefined, "Right");

    var vp = dlg.add("panel", undefined, "Vertical");
    vp.orientation = "row";
    vp.alignChildren = "fill";
    vp.margins = 12;
    var vT = vp.add("button", undefined, "Top");
    var vM = vp.add("button", undefined, "Middle");
    var vB = vp.add("button", undefined, "Bottom");

    function bind(btn, mode) {
        btn.onClick = function () { applyAlign(mode, anchor(), prev.value); };
    }
    bind(hL, "left");   bind(hC, "hcenter"); bind(hR, "right");
    bind(vT, "top");    bind(vM, "vcenter"); bind(vB, "bottom");

    var foot = dlg.add("group");
    foot.alignment = "right";
    var done = foot.add("button", undefined, "Done");
    done.onClick = function () { dlg.close(); };

    dlg.show();

    try { app.selection = restore; } catch (e) {}
})();
