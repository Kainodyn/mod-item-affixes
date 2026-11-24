#include "Chat.h"
#include "CommandScript.h"
#include "Item.h"
#include "ItemAffix.h"
#include "Player.h"
#include "SpellAuras.h"
#include "SpellInfo.h"
#include "SpellMgr.h"
#include "StringConvert.h"
#include "StringFormat.h"

using namespace Acore::ChatCommands;

namespace
{
char const* const SlotNames[EQUIPMENT_SLOT_END] = {
    "Head", "Neck", "Shoulders", "Shirt", "Chest", "Waist", "Legs", "Feet", "Wrist", "Hands",
    "Ring1", "Ring2", "Trinket1", "Trinket2", "Back", "MainHand", "OffHand", "Ranged", "Tabard",
};

std::string RoleNames(uint8 mask)
{
    std::string out;
    auto add = [&](uint8 bit, char const* name) { if (mask & bit) out += (out.empty() ? "" : ",") + std::string(name); };
    add(AFFIX_ROLE_CASTER, "caster");
    add(AFFIX_ROLE_PHYSICAL, "physical");
    add(AFFIX_ROLE_TANK, "tank");
    add(AFFIX_ROLE_HEALER, "healer");
    add(AFFIX_ROLE_RANGED, "ranged");
    return out.empty() ? "none" : out;
}

// Equipment slots are given 1-based, as shown by `.affix info`.
Item* EquippedItem(ChatHandler* handler, uint8 equipSlot)
{
    Player* player = handler->GetSession()->GetPlayer();
    Item* item = (equipSlot >= 1 && equipSlot <= EQUIPMENT_SLOT_END)
        ? player->GetItemByPos(INVENTORY_SLOT_BAG_0, uint8(equipSlot - 1)) : nullptr;
    if (!item)
        handler->PSendSysMessage("|cffFFFF00[ItemAffixes]|r No item in equipment slot {} (1-{}).", equipSlot, EQUIPMENT_SLOT_END);
    return item;
}
}

class ItemAffixCommandScript : public CommandScript
{
public:
    ItemAffixCommandScript() : CommandScript("ItemAffixCommandScript") { }

    ChatCommandTable GetCommands() const override
    {
        static ChatCommandTable reforgeCommandTable =
        {
            { "status", HandleReforgeStatusCommand, SEC_GAMEMASTER,     Console::No },
            { "roll",   HandleReforgeRollCommand,   SEC_ADMINISTRATOR, Console::No },
            { "pick",   HandleReforgePickCommand,   SEC_ADMINISTRATOR, Console::No },
        };
        static ChatCommandTable affixCommandTable =
        {
            { "info",    HandleInfoCommand,   SEC_GAMEMASTER,     Console::No },
            { "spec",    HandleSpecCommand,   SEC_GAMEMASTER,     Console::No },
            { "skills",  HandleSkillsCommand, SEC_GAMEMASTER,     Console::No },
            { "reroll",  HandleRerollCommand, SEC_ADMINISTRATOR, Console::No },
            { "give",    HandleGiveCommand,   SEC_ADMINISTRATOR, Console::No },
            { "reforge", reforgeCommandTable },
        };
        static ChatCommandTable commandTable =
        {
            { "affix", affixCommandTable },
        };
        return commandTable;
    }

    // .affix info -- affixes of every equipped item.
    static bool HandleInfoCommand(ChatHandler* handler)
    {
        Player* player = handler->GetSession()->GetPlayer();
        bool any = false;
        for (uint8 slot = EQUIPMENT_SLOT_START; slot < EQUIPMENT_SLOT_END; ++slot)
        {
            Item* item = player->GetItemByPos(INVENTORY_SLOT_BAG_0, slot);
            if (!item)
                continue;
            std::vector<AffixSlot> slots = sItemAffixMgr->GetSlots(item->GetGUID().GetRawValue());
            if (slots.empty())
                continue;
            any = true;
            handler->PSendSysMessage("|cffFFFF00[{} {}]|r {} (entry {}, guid {})", slot + 1, SlotNames[slot],
                item->GetTemplate()->Name1, item->GetEntry(), item->GetGUID().GetCounter());
            for (size_t i = 0; i < slots.size(); ++i)
            {
                AffixDefinition const* def = sItemAffixMgr->GetAffixDef(slots[i].affixId);
                if (slots[i].rollState == AFFIX_ROLL_APPLIED && def)
                    handler->PSendSysMessage("  [{}] {}{} (affix {})", i,
                        sItemAffixMgr->BuildAffixDisplayString(def, slots[i].rolledValue),
                        slots[i].isCrit ? " (crit)" : "", def->id);
                else
                    handler->PSendSysMessage("  [{}] empty (nothing was eligible)", i);
            }
            sItemAffixMgr->SendItemStatus(player, item);
        }
        if (!any)
            handler->SendSysMessage("|cffFFFF00[ItemAffixes]|r No equipped item has affixes.");
        return true;
    }

    // .affix spec -- the specialization profile new rolls use.
    static bool HandleSpecCommand(ChatHandler* handler)
    {
        AffixSpecProfile profile = CoASpecRoles::Resolve(handler->GetSession()->GetPlayer());
        handler->PSendSysMessage("|cffFFFF00[ItemAffixes]|r spec {} roles {}{} primary stats {} {} {}",
            profile.specId, RoleNames(profile.roleMask), profile.anyRole ? " (any role)" : "",
            profile.primaryStats[0], profile.primaryStats[1], profile.primaryStats[2]);
        return true;
    }

    // .affix skills -- skill affixes on equipped items and the spell modifiers they produce.
    static bool HandleSkillsCommand(ChatHandler* handler)
    {
        Player* player = handler->GetSession()->GetPlayer();
        ItemAffixPlayerData* data = player->CustomData.Get<ItemAffixPlayerData>("ItemAffixData");
        Aura* carrier = player->GetAura(SKILL_AFFIX_CARRIER_SPELL);
        handler->PSendSysMessage("|cffFFFF00[ItemAffixes]|r {} skill affix(es) loaded; carrier aura {} {}.",
            sItemAffixMgr->GetSkillAffixCount(), SKILL_AFFIX_CARRIER_SPELL, carrier ? "present" : "absent");
        if (!data)
            return true;

        for (auto const& [guid, affixes] : data->activeSkillAffixes)
            for (ActiveSkillAffix const& active : affixes)
                if (AffixDefinition const* def = sItemAffixMgr->GetAffixDef(active.affixId))
                    handler->PSendSysMessage("  item {}: [{}] {} {} ({}%){}", ObjectGuid(guid).GetCounter(), def->id,
                        def->key, sItemAffixMgr->BuildAffixDisplayString(def, active.scalePct), active.scalePct,
                        sItemAffixMgr->PlayerKnowsSkillAffix(player, *def) ? "" : " (inactive: spell not known)");

        for (SpellModifier const* mod : data->skillMods)
        {
            SpellInfo const* info = sSpellMgr->GetSpellInfo(mod->targetSpellId ? mod->targetSpellId : mod->spellId);
            handler->PSendSysMessage("  mod op {} {} {} spell {} {} mask {:#x}:{:#x}:{:#x} owner {}", uint32(mod->op),
                mod->type == SPELLMOD_PCT ? "pct" : "flat", mod->value, mod->targetSpellId ? mod->targetSpellId : mod->spellId,
                info ? info->SpellName[0] : "?", mod->mask[0], mod->mask[1], mod->mask[2],
                mod->ownerAura == carrier ? "carrier" : "STALE");
        }
        for (auto const& [spellId, cut] : data->cooldownCuts)
        {
            SpellInfo const* info = sSpellMgr->GetSpellInfo(spellId);
            handler->PSendSysMessage("  cooldown cut spell {} {}: -{} ms after cast", spellId,
                info ? info->SpellName[0] : "?", cut);
        }
        return true;
    }

    // .affix give <equipSlot> <affix id|key> [crit] -- put one skill affix on an equipped item (testing;
    // no class or known-spell check). Replaces the item's skill line, else adds a line.
    static bool HandleGiveCommand(ChatHandler* handler, uint8 equipSlot, std::string affix, Optional<std::string> crit)
    {
        Item* item = EquippedItem(handler, equipSlot);
        if (!item)
            return false;
        AffixDefinition const* def = nullptr;
        if (Optional<uint32> id = Acore::StringTo<uint32>(affix))
            def = sItemAffixMgr->GetAffixDef(*id);
        else
            def = sItemAffixMgr->GetAffixDefByKey(affix);
        if (!def || !def->IsSkill())
        {
            handler->PSendSysMessage("|cffFFFF00[ItemAffixes]|r Unknown skill affix '{}'.", affix);
            return false;
        }
        Player* player = handler->GetSession()->GetPlayer();
        if (!sItemAffixMgr->GiveSkillAffix(player, item, *def, crit && *crit == "crit"))
        {
            handler->SendSysMessage("|cffFFFF00[ItemAffixes]|r That item cannot carry affixes.");
            return false;
        }
        handler->PSendSysMessage("|cffFFFF00[ItemAffixes]|r {} now has [{}] {}{}.", item->GetTemplate()->Name1, def->id,
            def->key, sItemAffixMgr->PlayerKnowsSkillAffix(player, *def) ? "" : " (inactive: spell not known)");
        return true;
    }

    // .affix reroll <equipSlot> -- replace every affix of an equipped item and clear its reforge lock.
    static bool HandleRerollCommand(ChatHandler* handler, uint8 equipSlot)
    {
        Item* item = EquippedItem(handler, equipSlot);
        if (!item)
            return false;
        sItemAffixMgr->RerollItem(handler->GetSession()->GetPlayer(), item);
        handler->PSendSysMessage("|cffFFFF00[ItemAffixes]|r Rerolled {}.", item->GetTemplate()->Name1);
        return true;
    }

    static bool HandleReforgeStatusCommand(ChatHandler* handler, uint8 equipSlot)
    {
        Item* item = EquippedItem(handler, equipSlot);
        if (!item)
            return false;
        ReforgeState state = sItemAffixMgr->GetReforgeState(item->GetGUID().GetRawValue());
        uint32 cost = sItemAffixMgr->GetReforgeCost(item->GetTemplate()->Quality, state.exists ? state.rerollCount : 0);
        if (state.exists)
            handler->PSendSysMessage("|cffFFFF00[Reforge]|r locked to slot {}, reforged {} time(s), next cost {}c.",
                state.lockedSlot, state.rerollCount, cost);
        else
            handler->PSendSysMessage("|cffFFFF00[Reforge]|r never reforged, first cost {}c.", cost);
        return true;
    }

    static bool HandleReforgeRollCommand(ChatHandler* handler, uint8 equipSlot, uint8 affixSlot)
    {
        Item* item = EquippedItem(handler, equipSlot);
        if (!item)
            return false;
        std::vector<ReforgeOption> opts;
        ReforgeRollResult result = sItemAffixMgr->RollReforgeOptions(handler->GetSession()->GetPlayer(), item, affixSlot, &opts);
        if (result != ReforgeRollResult::OK)
        {
            handler->PSendSysMessage("|cffFFFF00[Reforge]|r failed ({}).", uint32(result));
            return false;
        }
        for (size_t i = 0; i < opts.size(); ++i)
            handler->PSendSysMessage("  [{}] {}{}", i,
                sItemAffixMgr->BuildAffixDisplayString(sItemAffixMgr->GetAffixDef(opts[i].affixId), opts[i].rolledValue),
                opts[i].isCrit ? " (crit)" : "");
        return true;
    }

    static bool HandleReforgePickCommand(ChatHandler* handler, uint8 equipSlot, uint8 affixSlot, uint32 optIdx)
    {
        Item* item = EquippedItem(handler, equipSlot);
        if (!item)
            return false;
        ReforgePickResult result = sItemAffixMgr->CommitReforgePick(handler->GetSession()->GetPlayer(), item, affixSlot, optIdx);
        handler->PSendSysMessage("|cffFFFF00[Reforge]|r {}.", result == ReforgePickResult::OK ? "applied" : "failed");
        return result == ReforgePickResult::OK;
    }
};

void AddSC_item_affix_commands()
{
    new ItemAffixCommandScript();
}
