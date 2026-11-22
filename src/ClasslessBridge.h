/*
 * mod-item-affixes — classless bridge
 *
 * A thin adapter between this module and mod-classless-wildcard. Every place
 * that used to ask "what class is this player?" asks here instead.
 *
 * mod-classless-wildcard turns players into "Heroes" on a fixed chassis class
 * (Paladin), so Player::getClass() says nothing about what a Hero can actually
 * do. For a Hero, the answers come from the Wildcard character state instead:
 * the abilities and talents it owns, and its primary stat allocation.
 *
 * When mod-classless-wildcard is not compiled into the same worldserver
 * (detected with __has_include), or the player is exempt from it (bots on the
 * exempt account prefix), every function answers from the real class, so the
 * module behaves exactly as upstream.
 */

#ifndef MOD_ITEM_AFFIXES_CLASSLESS_BRIDGE_H
#define MOD_ITEM_AFFIXES_CLASSLESS_BRIDGE_H

#include "Define.h"
#include <array>

class Player;

namespace AffixClassless
{
    // True when mod-classless-wildcard is compiled in.
    bool Available();

    // True when the player is a Wildcard Hero (not exempt, state loaded).
    bool IsHero(Player* player);

    // Bitmask (1 << (classId - 1)) of the classes the player draws from.
    // Non-Hero: its own class. Hero: every class it owns an ability or a
    // talent from.
    uint32 OwnedClassMask(Player* player);

    // The class and talent tree (0..2) the player has invested most in.
    // Non-Hero: real class, tree from its own talent map (-1 = none).
    // Hero: the talent tab holding the most owned talent ranks; when it owns
    // no talents, the class with the most owned abilities and tree -1.
    struct Spec
    {
        uint8 cls  = 0;   // 0 = unknown
        int8  tree = -1;  // -1 = none / any
    };
    Spec DominantSpec(Player* player);

    // Whether the player owns any rank of the talent (Talent.dbc id).
    bool OwnsTalent(Player* player, uint32 talentId);

    // The Hero's primary stat allocation, in Wildcard order
    // (STR, AGI, STA, INT, SPI). Returns false for non-Heroes and for Heroes
    // that have not allocated any points yet.
    bool StatAllocation(Player* player, std::array<uint32, 5>& out);
}

#endif
