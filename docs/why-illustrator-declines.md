# Why Illustrator declines

The measured answer to "why can I not subgroup two objects, and why will
*Align* not take one of them as an anchor?" Both fall out of a single rule,
and it is not the one everyone repeats.

Everything here was measured by driving the running application over its COM
bridge and reading the resulting object tree back, rather than taken from
documentation. Reproducing it needs only `New-Object -ComObject
Illustrator.Application` and `DoJavaScript`. Where a measurement says "by
hand", a person performed the gesture at the keyboard while the plugin
logged the resulting state; those are the ones that matter, for the reason
given in [the implementation notes](implementation-notes.md).

Measured against **Illustrator 30.7 (2026)** on Windows.

---

## The restriction is not what it looks like

The usual description is "you can't make a subgroup unless the group holds at
least three objects." That turns out to be a symptom, not the rule.

Driving the live application and reading back what the selection model actually
contains gives this:

| What was requested | What `app.selection` came back as | *Ctrl+G* result |
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

- **Subgrouping.** *Ctrl+G* never sees two objects; it sees one group, and
  grouping a lone group is suppressed as redundant (verified separately: a
  single selected group put through the *Group* command stays parented to its
  layer).
- **Align anchor.** A key object is designated by clicking an object that is
  already selected alongside others, so it needs two or more separately selected
  objects. When the group holds exactly the objects you want to align, the
  selection is one group and there is nothing to nominate.

## What about the *Layers* panel's two columns?

The obvious objection: the *Layers* panel already offers two ways to click an
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
state, *Horizontal Align Left* moved nothing at all — both objects' bounds were
byte-identical before and after — while the *Group* command produced no nesting.

The circle drives **appearance targeting**, a channel parallel to selection, and
it has no scripting surface whatsoever: reflecting over `PathItem`, `GroupItem`,
`Layer`, and `Document` turns up `selected`, `hasSelectedArtwork`, and
`isIsolated`, but no `targeted` of any kind. Neither ExtendScript nor UXP can
read which gesture was used.

`kArtTargeted` *is* readable from C++, which is the obvious next hope, and it is
also a dead end. Measured with the plugin reading the attribute bits directly:

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
the opposite: a flat, unambiguous set. Were *Ctrl+G* to consume targets, "both
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

In Illustrator, a group is itself a selectable, directly manipulable object. For
every direct-manipulation operation — drag, scale, rotate, delete, or transform
— "all children selected" and "the group selected" mean precisely the same thing
and produce precisely the same result. Keeping two distinct internal
representations of an identical manipulation state would need an answer for what
the bounding box and handles look like, what the *Appearance* panel targets, what
*Ctrl+G* means, and what *Ctrl+Shift+G* means, in each of them. Collapsing to the
group removes the ambiguity at the cost of making one state unrepresentable.

The suppression of *Ctrl+G* on an already-complete group has an obvious
companion motive: *Ctrl+G* is hammered, and without a guard every extra press
would add another invisible wrapper, each of which is a real transform and
clipping scope in the imaging model. The guard stops accidental depth. It also
stops deliberate depth, which is the actual complaint.

So it is a genuine design decision, not an oversight or a data-model limit — but
the decision is about the *selection model*, and it is enforced well upstream of
anything structural.

## Is there a drawback to bypassing it?

Measured, not assumed:

- **The file format is fine.** `Group(Group(A, B))` and even the fully
  degenerate `Group(Group(single path))` were saved to *.ai*, closed, and
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
addressable in the *Layers* panel, but it did not create a new selection state,
because nothing can — the rule lives in the selection model, upstream of the
commands, and not even the *Layers* panel's target column escapes it. This plugin
changes structure; it does not change what a click means.

The other consequence is the one Adobe's guard was protecting against: nesting
depth now costs you whatever you spend. Each level is a transform and clipping
scope, so hundreds of pointless levels will bloat a file and slow rendering.
That is now your decision rather than Illustrator's.

