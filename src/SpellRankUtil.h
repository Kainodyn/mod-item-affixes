#pragma once

#include "Player.h"
#include "SpellInfo.h"
#include "SpellMgr.h"

// Rank helpers shared by spell swaps and Imprints, so every custom effect that
// casts or mirrors a ranked spell uses the rank that fits the player instead
// of a hardcoded level-80 rank.
namespace AffixSpellRank
{
    // Highest rank of anySpellInChain whose required level (SpellLevel) is
    // <= level. 0 if even rank 1 is above that level.
    inline uint32 HighestForLevel(uint32 anySpellInChain, uint8 level)
    {
        uint32 best = 0;
        for (uint32 id = sSpellMgr->GetFirstSpellInChain(anySpellInChain); id; id = sSpellMgr->GetNextSpellInChain(id))
        {
            SpellInfo const* info = sSpellMgr->GetSpellInfo(id);
            if (info && info->SpellLevel <= level)
                best = id;
        }
        return best;
    }

    // Highest rank of anySpellInChain the player knows. 0 if none.
    inline uint32 HighestKnown(Player* player, uint32 anySpellInChain)
    {
        uint32 best = 0;
        for (uint32 id = sSpellMgr->GetFirstSpellInChain(anySpellInChain); id; id = sSpellMgr->GetNextSpellInChain(id))
            if (player->HasSpell(id))
                best = id;
        return best;
    }

    // Highest known rank, or the highest rank for the player's level when the
    // player knows none (e.g. the chain is temporarily replaced by a swap variant).
    inline uint32 Best(Player* player, uint32 anySpellInChain)
    {
        if (uint32 known = HighestKnown(player, anySpellInChain))
            return known;
        return HighestForLevel(anySpellInChain, player->GetLevel());
    }

    // Removes every rank of the chain the player knows. removeSpell already
    // removes all higher non-talent ranks recursively, so removing the lowest
    // known rank clears the whole chain.
    inline void RemoveChain(Player* player, uint32 anySpellInChain)
    {
        for (uint32 id = sSpellMgr->GetFirstSpellInChain(anySpellInChain); id; id = sSpellMgr->GetNextSpellInChain(id))
        {
            if (player->HasSpell(id))
            {
                player->removeSpell(id, SPEC_MASK_ALL, false);
                return;
            }
        }
    }
}
