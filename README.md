# Subgroup

An Adobe Illustrator plug-in that removes two long-standing restrictions on
grouping and alignment — without the junk-object workaround.

- **Nest Down** and **Nest Up** add a level of grouping inside or around the
  selection, including when the selection is an entire group, which
  Object > Group declines outright.
- **Align Group To Selected** aligns a group's contents to one object inside it,
  which Illustrator's own Align cannot reach.

Measured against **Illustrator 30.7 (2026)** on Windows. An ExtendScript
fallback is kept in `scripts/` for anyone who would rather not build anything.

---

## The restriction is not what it looks like

The usual description is "you can't make a subgroup unless the group holds at
least three objects." That turns out to be a symptom, not the rule.

Driving the live application and reading back what the selection model actually
contains gives this:

| What was requested | What `app.selection` came back as | `Ctrl+G` result |
| --- | --- | --- |
| Both children of a 2-object group | `len=1 [GroupItem]` | no-op |
| **All three** children of a 3-object group | `len=1 [GroupItem]` | **no-op** |
| Two of the three children of a 3-object group | `len=2 [PathItem, PathItem]` | subgroup created |
| One child of a 2-object group | `len=1 [PathItem]` | single-child group created |
| Two top-level items forming an entire *layer* | `len=2 [PathItem, PathItem]` | group created |

The third row kills the "needs three objects" theory: a three-object group is
just as stuck as a two-object one when you select all of it. The real rule is:

> **A selection consisting of every child of a group is a selection of the group
> itself.**

Not a rewrite, either — the same bits by construction. `AIArt.h` says a
container carries `kArtFullySelected` *only if* all its descendants are
selected, so "all children selected" and "the group selected" are one state with
one representation.

A two-object group is stuck *because* any selection of both objects is
necessarily the whole group. Add a third object and a two-object selection
becomes a proper subset, which survives — hence the junk-object workaround.

Two further results narrow it down. Illustrator will happily create a group
holding a single path (row 4), so this is not a rule against degenerate groups.
And a set of items making up a whole *layer* is **not** collapsed (row 5), so it
is not about containers generally. It is specific to groups.

Both of the original complaints fall out of that one rule:

- **Subgrouping.** `Ctrl+G` never sees two objects; it sees one group, and
  grouping a lone group is suppressed as redundant (verified separately: a
  single selected group put through the Group command stays parented to its
  layer).
- **Align anchor.** A key object is designated by clicking an object that is
  already selected alongside others, so it needs two or more separately selected
  objects. When the group holds exactly the objects you want to align, the
  selection is one group and there is nothing to nominate.

## What about the Layers panel's two columns?

The obvious objection: the Layers panel already offers two ways to click an
object, and they visibly differ. Clicking the **target circle** on each child
keeps them looking individually selected, while clicking **to the right of the
circle** resolves to the whole group. Adobe evidently built two mechanisms with
different semantics, so why not let the first one unlock grouping and aligning?

Because they are not two selection methods. Measured, with a person performing
the gesture by hand and the state read back over COM without anything else being
touched:

```
target-circle click on each child : selection = len=1 [GroupItem]   children .selected = true   group .selected = true
group selected directly           : selection = len=1 [GroupItem]   children .selected = true   group .selected = true
```

Indistinguishable. Every readable property is identical. And from that hand-made
state, `Horizontal Align Left` moved nothing at all — both objects' bounds were
byte-identical before and after — while the Group command produced no nesting.

The circle drives **appearance targeting**, a channel parallel to selection, and
it has no scripting surface whatsoever: reflecting over `PathItem`, `GroupItem`,
`Layer`, and `Document` turns up `selected`, `hasSelectedArtwork`, and
`isIsolated`, but no `targeted` of any kind. Neither ExtendScript nor UXP can
read which gesture was used.

`kArtTargeted` *is* readable from C++, which is the obvious next hope, and it is
also a dead end. Measured with the plug-in reading the attribute bits directly:

```
click the artwork      -> PAIR sel=1 full=1 TARGETED=1    targeted set = {PAIR}
click RED's circle     -> RED  sel=1 full=1 TARGETED=1    targeted set = {RED}
select group, then RED -> IDENTICAL to the line above
```

Clicking a target circle **replaces** the selection rather than annotating it,
so "both objects selected, one of them targeted" is not a representable state.
Targeting simply follows the selection and carries no extra information.

There is also a structural reason the two channels are not interchangeable.
Targeting is hierarchical and additive rather than exclusive: flagging a single
child makes the parent group report `selected = true` as well, because on a
container that property means *contains a selection*, not *is selected*. A group
and its children can be targeted at once, which for appearance is meaningful — a
drop shadow on the group plus a stroke on each child. Structural commands need
the opposite: a flat, unambiguous set. Were `Ctrl+G` to consume targets, "both
children targeted", "the group targeted", and "all three targeted" would all be
legal inputs, and the first two are exactly the ambiguity Illustrator declines to
resolve.

Adobe could still have made the individual-target state produce a flat
two-object selection as well. That is a defensible product call in either
direction — but it means defining a new selection state, not extending an
existing one.

## Why it is probably there

Adobe has never documented a rationale, so this part is inference rather than
measurement.

In Illustrator a group is itself a selectable, directly manipulable object. For
every direct-manipulation operation — drag, scale, rotate, delete, or transform
— "all children selected" and "the group selected" mean precisely the same thing
and produce precisely the same result. Keeping two distinct internal
representations of an identical manipulation state would need an answer for what
the bounding box and handles look like, what the Appearance panel targets, what
`Ctrl+G` means, and what `Ctrl+Shift+G` means, in each of them. Collapsing to the
group removes the ambiguity at the cost of making one state unrepresentable.

The suppression of `Ctrl+G` on an already-complete group has an obvious
companion motive: `Ctrl+G` is hammered, and without a guard every extra press
would add another invisible wrapper, each of which is a real transform and
clipping scope in the imaging model. The guard stops accidental depth. It also
stops deliberate depth, which is the actual complaint.

So it is a genuine design decision, not an oversight or a data-model limit — but
the decision is about the *selection model*, and it is enforced well upstream of
anything structural.

## Is there a drawback to bypassing it?

Measured, not assumed:

- **The file format is fine.** `Group(Group(A, B))` and even the fully
  degenerate `Group(Group(single path))` were saved to `.ai`, closed, and
  reopened with nesting, names, and z-order all intact.
- **Illustrator already builds these shapes itself.** Grouping two existing
  groups nests groups; single-child groups are creatable through the normal UI.
- **Undo is one step.** Running the command and pressing undo once reverted the
  entire nest and stopped there, leaving the art alone.
- **Z-order is preserved exactly.** Wrapping a non-contiguous selection
  reproduces native placement byte for byte: the new group takes the z-position
  of the topmost selected item, and members keep their relative order.

The one real consequence is unavoidable and worth knowing: **the selection rule
still applies afterward.** After nesting `Group(A, B)` into `Group(Group(A, B))`,
clicking the art still resolves to the outer group. The new level is real and
addressable in the Layers panel, but it did not create a new selection state,
because nothing can — the rule lives in the selection model, upstream of the
commands, and not even the Layers panel's target column escapes it. This plug-in
changes structure; it does not change what a click means.

The other consequence is the one Adobe's guard was protecting against: nesting
depth now costs you whatever you spend. Each level is a transform and clipping
scope, so hundreds of pointless levels will bloat a file and slow rendering.
That is now your decision rather than Illustrator's.

---

## The plug-in — `plugin/`

Everything lives in one submenu, **Object > Subgroup**, placed directly above
Illustrator's own Group/Ungroup block:

```
Nest Down
Nest Up
──────────────────────────
Nest Whole Groups        ✓
Keep New Groups Open
──────────────────────────
Align Group To Selected ▸   Left
                            Horizontal Center
                            Right
                            ──────────────────
                            Center
                            ──────────────────
                            Top
                            Vertical Center
                            Bottom
```

### Nest Down

What `Ctrl+G` should have done. With the selection an entire group, it adds a
level *inside* that group holding everything the group currently contains.
Anywhere else it groups the selection exactly as Illustrator would, reproducing
native placement rather than imitating it.

Every command here is assignable under **Edit > Keyboard Shortcuts… > Menu
Commands > Object > Subgroup**, like any other menu command. Nest Down is the
one worth a key: give it `Ctrl+G` and grouping simply starts working where
Illustrator declines, with nothing else changed. The plug-in deliberately does
not claim any shortcut for you — see [The shortcut has to be assigned by
hand](#the-shortcut-has-to-be-assigned-by-hand).

### Nest Up

The mirror: adds a level *around* the selection, including around a lone group.
Same resulting shape as Nest Down, different group object left outermost, which
matters if the original carries an appearance.

### Align Group To Selected

Select **one object inside a group** and its siblings move to meet it. Seven
alignments, with combined-center in its own division between the horizontal and
vertical triples, matching where Adobe puts it in the control bar.

This is the one thing vanilla Illustrator cannot do. Everywhere else its key
object covers it: with a *proper subset* of a group selected you can click one
of them with the Selection tool to make it the key, and Object > Align works.
What it cannot reach is a group's contents as a whole, because selecting all of
them *is* selecting the group — one object, so Align shifts the group against
the artboard instead of arranging what is inside it.

The selection already says which object should hold still, so there is no anchor
to set and nothing to remember. Earlier versions had Set / Clear Align Anchor
commands feeding Illustrator's own key object; they were removed as redundant
once it was established that vanilla can nominate a key object in every case
where Align is capable of using one.

Alignment uses `kNoStrokeBounds | kExcludeGuideBounds` — geometric bounds, what
the Align panel uses with *Use Preview Bounds* off — and moves each object with
a translation matrix through `TransformArt`. Locked and hidden siblings are
skipped rather than reported.

The commands gray out when they do not apply, the same way Object > Group does.
Align in particular needs exactly one child of one group selected: a whole group
singles out nothing, and picks spread across two groups are ambiguous. Partial
selection counts, so the Direct Selection tool works.

### The toggles

Both are checkable items, persisted in `AIPreferenceSuite`.

- **Nest Whole Groups** — governs the headline behavior. Off, Nest Down declines
  a whole-group selection exactly as Illustrator does. On by default.
- **Keep New Groups Open** — leaves a newly made group expanded in the Layers
  panel instead of collapsed. Off by default, which is what vanilla grouping
  does.

Ancestors of a new group are *always* re-expanded regardless of the toggle. That
is the one piece of native behavior deliberately not reproduced: adding a group
several levels down collapses the topmost one rather than the new one, so the
artwork appears to vanish from the Layers panel. Re-opening the path leaves only
the newest group closed, which is what the collapse was presumably meant to do.

### Install

Point **Preferences > Plug-ins & Scratch Disks > Additional Plug-ins Folder** at
`install/` and restart Illustrator. No admin rights, and deleting the `.aip`
uninstalls it. Swapping the file needs Illustrator fully closed — it holds the
plug-in open while running.

### Build

Visual Studio 2022 (`v143` toolset) against the Illustrator 2026 SDK
(Build 114). The project carries no machine-specific path, so tell it where the
SDK is in any one of three ways:

```
msbuild plugin\Subgroup.vcxproj /p:Configuration=Release /p:Platform=x64 /p:AISDK="<path to SDK>"
```

…or set an `AISDK` environment variable, or drop a `plugin\AISDK.props` beside
the project:

```xml
<?xml version="1.0" encoding="utf-8"?>
<Project xmlns="http://schemas.microsoft.com/developer/msbuild/2003">
  <PropertyGroup>
    <AISDK>C:\path\to\Adobe Illustrator 2026 SDK</AISDK>
  </PropertyGroup>
</Project>
```

That file is not tracked. With none of the three set, the build stops with a
readable message rather than a wall of missing-header errors.

Note the SDK is version-gated per suite rather than by a declared interface
version, so a CS6 SDK cannot produce a plug-in that loads into 30.7 — the v30
SDK from the Adobe Developer Console is required.

### Looking at the About dialog — `tools/AboutHarness/`

A modal dialog inside a host application is close to untestable: you cannot
drive it, screenshot it, or measure it without a person sitting in front of it.
So `SubgroupAbout.cpp` is written free of every Illustrator type — it takes a
module handle and a parent window and touches nothing else — and
`tools/AboutHarness/build.cmd` compiles that same file and that same dialog
resource into a small executable that just shows it.

```
tools\AboutHarness\build.cmd     from a Visual Studio x64 command prompt
AboutHarness.exe                 show the dialog
AboutHarness.exe /exit3000       show it, then close after three seconds
```

The harness `#include`s the plug-in's dialog template rather than copying it. A
copy would stop being evidence about what ships.

---

## Dead ends, measured

Four approaches that look like they should work and do not. Written down because
each one costs a day to rediscover.

### The command notifiers cannot carry this feature

The first design listened to `kAIGroupCommandPreNotifierStr` /
`…PostNotifierStr` around Object > Group, on the theory that `Ctrl+G` need never
be rebound. **That does not work.**

Illustrator does not dispatch Object > Group *at all* when the selection is
already a single group. Measured by hand at the keyboard: grouping two loose
rectangles fires `Before Group` and `After Group` normally, and pressing
`Ctrl+G` again on the resulting group fires **nothing**. The command is
short-circuited before it runs, so there is no notifier in exactly the case this
plug-in exists to handle.

This is easy to miss, because `executeMenuCommand("group")` *does* fire both
notifiers — it force-dispatches and bypasses the enablement check. A scripted
test therefore passes while the real keystroke does nothing. **Test the keyboard
path, not the scripting path.** The notifiers are still registered, but only so
the diagnostic build can log whether the native command ran.

### The shortcut has to be assigned by hand

`AIMenuSuite::SetItemCmd` is not a way out. Calling `SetItemCmd(item, 'g', 0)`
returns `kNoErr` and `GetItemCmd` reads the value back correctly, but
Illustrator does not honor a plug-in's claim over a shortcut an existing
built-in owns: `Ctrl+G` still reaches Object > Group. It reports success and
silently declines.

Worse, it re-asserts that conflicting binding at every launch, quietly editing
the user's saved keyboard set. The call is deliberately absent from this
plug-in and should stay absent.

`.kys` files are plain PostScript-style text, so a keyboard set can be inspected
directly: `/Menus { /<command name> { /Context /Modifiers /Represent /Key } }`,
where Modifiers 64 is Ctrl and Key 71 is `G`. This is also why
`kSubgroupGroupCmd` must never be renamed — it is the key the assignment is
stored under, and changing it silently orphans the user's `Ctrl+G`.

### A plug-in cannot add items to a native submenu's group

`AddMenuGroup` anchored near `kAlignObjectMenuGroup` returns `kNoErr`, and then
adding items to that group fails with `kBadParameterErr` (1346458189 =
`0x5041524D` = `'PARM'`). The group is created and is useless. The align
commands live in the plug-in's own submenu for this reason.

The related trap: an early `return` on that failure cost every *later* menu item
and both notifiers, silently disabling the whole plug-in. Notifiers are now
registered first, and optional menu placement cannot take the core down with it.

### Menu items lay out in insertion order

Menu *groups* control separators, not sequence. Items appear in the order they
are added, so the combined-center entry is inserted between the two triples
rather than after them, despite its own group being created between theirs.

---

## Traps worth knowing before extending it

**Illustrator's preference getters cannot express "never written."** Measured on
30.7, `GetBooleanPreference` returns `kNoErr` — success — with `false` for a key
that was never set, so a fallback argument can never fire. ExtendScript's
equivalent has the same defect pointing the other way, returning `true`. Neither
can carry a default. Every flag here is therefore stored in the sense where the
unset reading, `false`, *is* the wanted default: `nestingDisabled`, not
`nestingEnabled`.

**A layer is itself backed by a group art object.** It appears in
`GetSelectedArt` as a group with a null parent, so a naive "outermost selected
object" walk picks the layer and nests the whole layer's contents. Skip it with
`AIArtSuite::IsArtLayerGroup`, and do not let it count as a selected parent
either, or every real top-level object looks nested.

**"Did the command act?" cannot be answered by first-child identity.** Grouping
a subset puts the new group at the topmost selected index, which is often not
zero, so the first child is unchanged and a plug-in wrongly concludes the
command declined — adding a spurious level to ordinary grouping. The
discriminator is `kArtFullySelected`.

**`GetMatchingArt` with `kArtSelectedTopLevelGroups` returned nothing** here
(`err=0, n=0`), which is why the top level of the selection is derived from
`GetSelectedArt` by hand.

**Key art does not survive a hand-made selection change.** Illustrator cancels
it on every one, so a one-shot `SetKeyArt` never reaches the moment the user
opens Align. This is invisible to a scripted test, because setting
`app.selection` from the DOM does not cancel key art the way clicking does.

**`SDKAboutPluginsHelper::PopAboutBox` cannot carry non-ASCII text.** It takes a
`char*` and builds an `ai::UnicodeString` from it with the default encoding,
which is the *platform* one — so an em dash or a copyright sign becomes mojibake
on any machine whose code page is not Latin-1, and the author never sees it
because their own machine usually is.

It also ends in `AIUserSuite::MessageAlert`, a plain OS alert: one run of
unstyled text, no emphasis, no links. Windows' task dialog can hold links, but
only in its content and footer — its main instruction, the one piece of text
with any visual weight, cannot be one, and nothing in it can be emphasized.
Neither offers italics at all. So the About box here is an ordinary dialog
resource with `SysLink` controls where a link is wanted and a bold font where
weight is wanted, and its strings are UTF-16 with the non-ASCII characters
written as escapes, so no compiler has to guess at a source file's encoding.

Command names are bold rather than underlined on purpose: underline reads as
*clickable* to anyone who has used a computer, and these are not.

**DOM-driven verification does not reproduce hand gestures**, and this is the
methodological lesson of the whole project. `executeMenuCommand` is not
`Ctrl+G`; `app.selection = [...]` is not a click — it never leaves an object
partially selected, and `app.selection = null` triggers deselect handling a
click does not. Every one of the dead ends above passed a scripted test before
failing in the hand.

---

## The scripts (ExtendScript) — `scripts/`

Kept as the no-SDK fallback, and still the only route if you would rather not
build anything. These *do* rebind `Ctrl+G`, which the plug-in avoids.

Two commands and a settings dialog, sharing one persistent store.

### `Group.jsx` — the augmented Ctrl+G

Assign `Ctrl+G` to this and grouping starts working where Illustrator declines.

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

### `Settings.jsx` — the toggles

Augmented grouping on/off, and whether the new level goes inside the group or
around it.

Stored in `app.preferences` as `"1"` / `"0"` strings, not booleans, for the
reason given above: only `getStringPreference` returns a usable "unset" value.

### `Align To Anchor.jsx`

Aligns objects to an anchor picked from a list, replacing the key-object gesture
rather than trying to reach it. Select the **group** and run it: the dialog lists
the group's children, you pick the anchor, and the other objects move to meet it.
Clicking a row highlights that object on the canvas, which is the only practical
way to tell two unnamed paths apart. An ordinary selection of two or more objects
works too.

Six alignments, with a *Use preview bounds* checkbox mirroring the Align panel's
option. The dialog stays open so a horizontal and a vertical alignment can be
applied in one go.

### Install

Copy the folder itself — keeping `lib/` inside it — into Illustrator's Scripts
folder:

```
C:\Program Files\Adobe\Adobe Illustrator 2026\Presets\en_US\Scripts\Subgroup\
```

Restart Illustrator. Because it is a subfolder, the entries appear together
under **File > Scripts > Subgroup** rather than scattered through the menu.
`lib/` must stay beside the scripts; it is pulled in with a relative `#include`,
and the `.jsxinc` extension keeps those files out of the menu.

Binding `Ctrl+G` is safe for the reason described above — the script's first
action is to call the very command it just took the shortcut from — but it is a
global reassignment rather than a hook. If the folder is moved or deleted
without clearing the shortcut, `Ctrl+G` stops working until you restore it. That
is the honest cost of the scripting route, and the reason the plug-in exists.

---

## Known limits

- **A click on nested art still resolves to the outermost group.** The plug-in
  changes structure, not what a selection means; see above.
- Locked or hidden art, and art on a locked or hidden layer, cannot be
  reparented or moved. Nest Down stays silent about it, matching vanilla; Align
  skips those siblings.
- Nest Up needs all selected objects to share one parent. Illustrator's own
  Group command handles a cross-container selection by hoisting everything into
  a common ancestor; that case already works natively, so it is left alone.
- Windows x64 only as built. The project also declares ARM64 configurations, but
  nothing here has been tested on that platform or on macOS.

## License

**GNU General Public License, version 3 or later** (`GPL-3.0-or-later`), with an
**Adobe Illustrator SDK linking exception**. The full text is in
[LICENSE](LICENSE), the exception is in
[LICENSE-EXCEPTION](LICENSE-EXCEPTION), every source file carries an SPDX
header, and the About box shows the notice.

The exception is not decoration. An Illustrator plug-in cannot be built without
compiling Adobe's sample framework *into itself* — in a release build here,
seven of the nine object files are Adobe's, carrying a license that permits use
only on Adobe's terms. GPL section 10 forbids passing on a restriction like
that, and section 5(c) wants the whole combined work under the GPL, which
nobody but Adobe could grant for Adobe's code. Section 7 exists for this, and
this project uses it: the source stays GPL, and the combination with the SDK is
explicitly permitted so the compiled `.aip` can be distributed at all.

The repository itself vendors no Adobe code — the SDK is referenced by path —
and `scripts/` links nothing, so the ExtendScript never needed the exception in
the first place.

Copyright © 2026 Vixen420.

## Verification

Everything above was measured by driving the running application over its COM
bridge and reading the resulting object tree back, rather than from
documentation. Reproducing it needs only `New-Object -ComObject
Illustrator.Application` and `DoJavaScript`.

Where a measurement says "by hand", it means exactly that — a person performing
the gesture at the keyboard while the plug-in logged the resulting state. Those
are the measurements that matter here, for the reason given under dead ends.
