/*
 * mod-item-affixes — classless bridge. See ClasslessBridge.h.
 */

#include "ClasslessBridge.h"

#include "DBCStores.h"
#include "Player.h"
#include "SpellMgr.h"

#include <map>

#if defined(__has_include)
#  if __has_include("ClasslessMgr.h")
#    include "ClasslessMgr.h"
#    define AFFIX_HAS_CLASSLESS 1
#  endif
#endif

#ifndef AFFIX_HAS_CLASSLESS
#  define AFFIX_HAS_CLASSLESS 0
#endif

namespace
{
    uint8 ClassFromMask(uint32 mask)
    {
        for (uint8 cls = 1; cls < MAX_CLASSES; ++cls)
            if (mask & (1u << (cls - 1)))
                return cls;
        return 0;
    }

    // Upstream's dominant-tree rule for a normal character: the tree of its
    // own class holding the most talent ranks in the active spec.
    int8 NativeDominantTree(Player* player)
    {
        uint32 const* tabPages = GetTalentTabPages(player->getClass());
        if (!tabPages)
            return -1;

        int counts[3] = { 0, 0, 0 };
        for (auto const& [spellId, talent] : player->GetTalentMap())
        {
            if (!talent || talent->State == PLAYERSPELL_REMOVED)
                continue;
            if (!talent->IsInSpec(player->GetActiveSpec()))
                continue;
            TalentSpellPos const* pos = GetTalentSpellPos(spellId);
            if (!pos)
                continue;
            TalentEntry const* entry = sTalentStore.LookupEntry(pos->talent_id);
            if (!entry)
                continue;
            for (int t = 0; t < 3; ++t)
            {
                if (entry->TalentTab == tabPages[t])
                {
                    counts[t] += pos->rank + 1;
                    break;
                }
            }
        }

        int best = 0;
        for (int i = 1; i < 3; ++i)
            if (counts[i] > counts[best])
                best = i;
        return (counts[best] > 0) ? static_cast<int8>(best) : int8(-1);
    }

#if AFFIX_HAS_CLASSLESS
    // The loaded, non-exempt Wildcard state, or nullptr.
    ClasslessWildcard::CharState const* HeroState(Player* player)
    {
        if (!player || !sClasslessMgr->cfg.enabled)
            return nullptr;
        ClasslessWildcard::CharState const* st = sClasslessMgr->FindState(player);
        if (!st || st->exempt)
            return nullptr;
        return st;
    }
#endif
}

namespace AffixClassless
{
    bool Available()
    {
        return AFFIX_HAS_CLASSLESS != 0;
    }

    bool IsHero(Player* player)
    {
#if AFFIX_HAS_CLASSLESS
        return HeroState(player) != nullptr;
#else
        (void)player;
        return false;
#endif
    }

    uint32 OwnedClassMask(Player* player)
    {
        if (!player)
            return 0;
#if AFFIX_HAS_CLASSLESS
        if (auto const* st = HeroState(player))
        {
            uint32 mask = 0;
            for (auto const& [firstSpell, owned] : st->abilities)
                if (auto const* e = sClasslessMgr->GetAbility(firstSpell))
                    mask |= e->classMask;
            for (auto const& [talentId, rank] : st->talents)
                if (rank)
                    if (auto const* t = sClasslessMgr->GetTalent(talentId))
                        mask |= t->classMask;
            return mask;
        }
#endif
        return 1u << (player->getClass() - 1u);
    }

    Spec DominantSpec(Player* player)
    {
        Spec spec;
        if (!player)
            return spec;
#if AFFIX_HAS_CLASSLESS
        if (auto const* st = HeroState(player))
        {
            std::map<uint32, uint32> perTab;
            for (auto const& [talentId, rank] : st->talents)
                if (rank)
                    if (auto const* t = sClasslessMgr->GetTalent(talentId))
                        perTab[t->tabId] += rank;

            uint32 bestTab = 0, bestCount = 0;
            for (auto const& [tab, count] : perTab)
                if (count > bestCount)
                    bestTab = tab, bestCount = count;

            if (TalentTabEntry const* tab = bestTab ? sTalentTabStore.LookupEntry(bestTab) : nullptr)
            {
                spec.cls  = ClassFromMask(tab->ClassMask);
                spec.tree = tab->tabpage < 3 ? static_cast<int8>(tab->tabpage) : int8(-1);
                return spec;
            }

            // No talents yet: the class most of its abilities come from.
            std::map<uint8, uint32> perClass;
            for (auto const& [firstSpell, owned] : st->abilities)
                if (auto const* e = sClasslessMgr->GetAbility(firstSpell))
                    if (uint8 cls = ClassFromMask(e->classMask))
                        ++perClass[cls];
            uint32 best = 0;
            for (auto const& [cls, count] : perClass)
                if (count > best)
                    spec.cls = cls, best = count;
            return spec;
        }
#endif
        spec.cls  = player->getClass();
        spec.tree = NativeDominantTree(player);
        return spec;
    }

    bool OwnsTalent(Player* player, uint32 talentId)
    {
        if (!player || !talentId)
            return false;
#if AFFIX_HAS_CLASSLESS
        if (auto const* st = HeroState(player))
        {
            auto itr = st->talents.find(talentId);
            return itr != st->talents.end() && itr->second > 0;
        }
#endif
        TalentEntry const* entry = sTalentStore.LookupEntry(talentId);
        if (!entry)
            return false;
        for (uint32 rankSpell : entry->RankID)
            if (rankSpell && player->HasTalent(rankSpell, player->GetActiveSpec()))
                return true;
        return false;
    }

    bool StatAllocation(Player* player, std::array<uint32, 5>& out)
    {
#if AFFIX_HAS_CLASSLESS
        if (auto const* st = HeroState(player))
        {
            uint32 total = 0;
            for (size_t i = 0; i < out.size(); ++i)
                total += (out[i] = st->statAlloc[i]);
            return total > 0;
        }
#else
        (void)player;
        (void)out;
#endif
        return false;
    }
}
