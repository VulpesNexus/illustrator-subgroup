# Subgroup — the ExtendScript version

The no-SDK fallback, and the only route if you would rather not build anything.
These *do* rebind *Ctrl+G*; [the plugin](../README.md) avoids that.

Two commands and a settings dialog, sharing one persistent store.

## *Group.jsx* — the augmented Ctrl+G

Assign *Ctrl+G* to this and grouping starts working where Illustrator declines.

It is built around one rule: **Illustrator gets first refusal.** The script calls
the native Group command first and watches whether a probe item was reparented.
If it was, the script returns immediately — so every case that already worked is
not an imitation of vanilla behavior, it *is* vanilla behavior. Only after
watching the native command decline does it step in.

Three further rules follow from standing in for a reflex keystroke:

- **Never raise a dialog.** Vanilla fails silently when it cannot group; a
  stand-in that nags on a reflex press would be worse than the problem. Locked
  or hidden art makes it a quiet no-op.
- **Never throw.** The whole augmented path is guarded.
- **Off means off.** With the toggle off the script's entire body is one call to
  the native command.

Verified across six cases: the blocked case now nests; an ordinary 2-of-3
selection still produces byte-identical native placement; the toggle off leaves
structure untouched; wrap mode works; a locked child is a silent no-op; and an
empty selection does not throw.

## *Settings.jsx* — the toggles

Augmented grouping on/off, and whether the new level goes inside the group or
around it.

Stored in `app.preferences` as `"1"` / `"0"` strings rather than booleans:
Illustrator's preference getters have no usable "unset" sentinel, and only
`getStringPreference` returns anything a default can be told apart from. See
[implementation notes](../docs/implementation-notes.md).

## *Align To Anchor.jsx*

Aligns objects to an anchor picked from a list, replacing the key-object gesture
rather than trying to reach it. Select the **group** and run it: the dialog lists
the group's children, you pick the anchor, and the other objects move to meet it.
Clicking a row highlights that object on the canvas, which is the only practical
way to tell two unnamed paths apart. An ordinary selection of two or more objects
works too.

Six alignments, with a *Use preview bounds* checkbox mirroring the Align panel's
option. The dialog stays open so a horizontal and a vertical alignment can be
applied in one go.

## Install

Copy the folder itself — keeping *lib/* inside it — into Illustrator's Scripts
folder:

```
C:\Program Files\Adobe\Adobe Illustrator 2026\Presets\en_US\Scripts\Subgroup\
```

Restart Illustrator. Because it is a subfolder, the entries appear together
under *File > Scripts > Subgroup* rather than scattered through the menu.
*lib/* must stay beside the scripts; it is pulled in with a relative `#include`,
and the *.jsxinc* extension keeps those files out of the menu.

Binding *Ctrl+G* is safe because the script's first action is to call the very
command it just took the shortcut from — but it is a global reassignment rather
than a hook. If the folder is moved or deleted without clearing the shortcut,
*Ctrl+G* stops working until you restore it. That is the honest cost of the
scripting route, and the reason the plugin exists.

