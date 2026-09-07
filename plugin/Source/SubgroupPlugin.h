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
// every plugin built from it. See LICENSE-EXCEPTION.

#ifndef __SUBGROUPPLUGIN_H__
#define __SUBGROUPPLUGIN_H__

#include "SubgroupSuites.h"
#include "Plugin.hpp"
#include "AIMenuGroups.h"
#include "SDKDef.h"
#include "SDKAboutPluginsHelper.h"
#include "SubgroupID.h"

#include <vector>

Plugin* AllocatePlugin(SPPluginRef pluginRef);
void FixupReload(Plugin* plugin);

/** Grouping that also works when the selection is an entire group.

    Adds its own Group command rather than extending Object > Group, because
    Illustrator does not dispatch that command at all when the selection is
    already a single group: measured at the keyboard, neither the pre- nor the
    post-command notifier fires, so there is nothing to listen to in exactly the
    case this plugin exists for.

    The shortcut is left to the user's keyboard set. The plugin deliberately
    does not call SetItemCmd - see AddMenus for why that is worse than useless.
*/
class SubgroupPlugin : public Plugin
{
public:
    SubgroupPlugin(SPPluginRef pluginRef);
    virtual ~SubgroupPlugin() {}

    FIXUP_VTABLE_EX(SubgroupPlugin, Plugin);

protected:
    virtual ASErr Message(char* caller, char* selector, void* message);
    virtual ASErr StartupPlugin(SPInterfaceMessage* message);
    virtual ASErr Notify(AINotifierMessage* message);
    virtual ASErr GoMenuItem(AIMenuMessage* message);
    virtual ASErr UpdateMenuItem(AIMenuMessage* message);

private:
    enum AlignMode { kAlignLeft = 0, kAlignHCenter, kAlignRight,
                     kAlignTop, kAlignVCenter, kAlignBottom,
                     /** Both axes at once, matching the combined center align
                         Adobe added to the Align controls recently. */
                     kAlignBoth };
    AINotifierHandle    fGroupPreNotifier;
    AINotifierHandle    fGroupPostNotifier;

    /** Our own Group command. Assign Ctrl+G to it in Edit > Keyboard Shortcuts. */
    AIMenuItemHandle    fGroupCmdMenu;

    AIMenuItemHandle    fToggleMenu;
    AIMenuItemHandle    fWrapMenu;
    AIMenuItemHandle    fKeepOpenMenu;
    AIMenuItemHandle    fAlignMenu[7];
    /** The mode each entry performs, carried alongside the handle so the menu
        can be reordered without the dispatch silently going with it. */
    AlignMode           fAlignMode[7];

    AIMenuItemHandle    fProbeMenu;
    AIMenuItemHandle    fLogMenu;
    AIMenuItemHandle    fAboutPluginMenu;

    ASErr AddMenus(SPInterfaceMessage* message);
    ASErr AddNotifiers(SPInterfaceMessage* message);

    /** Reads a stored flag. An unwritten key reads false, which every key here
        is arranged to make the wanted default - see SubgroupID.h. */
    AIBoolean GetFlag(const char* suffix) const;
    void      PutFlag(const char* suffix, AIBoolean value) const;

    AIBoolean AugmentedNestingOn()  const { return !GetFlag(kSubgroupPrefNestingOff); }
    AIBoolean KeepNewGroupsOpen()   const { return  GetFlag(kSubgroupPrefKeepOpen); }

    /** Adds a level inside the group, holding everything it currently contains. */
    ASErr NestContents(AIArtHandle group);

    /** Settles the Layers panel disclosure state around a group we just made.

        Ancestors are always re-opened: Illustrator otherwise collapses the
        whole branch, so a group added several levels down closes the topmost
        one and the artwork appears to vanish. Only the new group's own state
        is a matter of taste, and that is what the Keep New Groups Open toggle
        governs - off means it arrives collapsed, exactly as vanilla grouping
        leaves it. */
    void SettleDisclosure(AIArtHandle newGroup) const;

    /** Writes the selected / fully-selected / targeted state of every relevant
        art object to the log. Always writes, regardless of the log toggle,
        because it exists only to be read. */
    static void DumpSelectionState();

    /** One-shot dump of every menu item and its group, for working out where
        our own items land. Only runs when the diagnostic log is on. */
    static void DumpMenus();

    /** The outermost selected objects, ignoring the layer's backing group and
        anything nested inside another selected object. */
    static void CollectTopLevelSelection(std::vector<AIArtHandle>& out);

    /** Everything Ctrl+G should do: nest when the selection is a whole group,
        otherwise group the selection the way Illustrator would have. */
    ASErr DoGroup();

    /** Adds a level above the selection, including around a lone group. */
    ASErr DoWrap();

    /** The About box, carrying the license notice the GPL asks an interactive
        program to show. Windows' task dialog where it is available, since it is
        the only dialog reachable from here that can hold a clickable link;
        AIUserSuite::MessageAlert otherwise. */
    static void ShowAboutBox();


    AIBoolean IsAlignItem(AIMenuItemHandle item) const;

    /** Moves the siblings of the selected object to meet it. */
    ASErr DoAlign(AlignMode mode);

    /** True when in-group alignment applies, and if so what to align to what.

        Exactly one CHILD of a group must be selected; it is the anchor and its
        siblings move to meet it. Partial selection counts, so picking with the
        Direct Selection tool works.

        This is the only case Illustrator cannot serve. With a proper subset of
        a group selected its own key object handles it, and a whole group offers
        nothing to single out. */
    AIBoolean InGroupAlignAvailable(std::vector<AIArtHandle>* kidsOut,
                                    AIArtHandle* anchorOut) const;

    /** Geometric bounds, stroke excluded, as the Align panel uses by default. */
    static ASErr BoundsOf(AIArtHandle art, AIRealRect* out);
    /** Reproduces native grouping: a new group at the topmost selected item's
        position in the common parent, members keeping their relative order. */
    ASErr GroupItems(const std::vector<AIArtHandle>& items);
};

#endif /* __SUBGROUPPLUGIN_H__ */
