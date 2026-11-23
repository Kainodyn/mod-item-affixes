#include "Chat.h"
#include "CommandScript.h"
#include "Item.h"
#include "ItemAffix.h"
#include "Player.h"
#include "RBAC.h"
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
            { "status", HandleReforgeStatusCommand, rbac::RBAC_PERM_COMMAND_GM, Console::No },
            { "roll",   HandleReforgeRollCommand,   rbac::RBAC_PERM_COMMAND_GM, Console::No },
            { "pick",   HandleReforgePickCommand,   rbac::RBAC_PERM_COMMAND_GM, Console::No },
        };
        static ChatCommandTable affixCommandTable =
        {
            { "info",    HandleInfoCommand,   rbac::RBAC_PERM_COMMAND_GM, Console::No },
            { "spec",    HandleSpecCommand,   rbac::RBAC_PERM_COMMAND_GM, Console::No },
            { "reroll",  HandleRerollCommand, rbac::RBAC_PERM_COMMAND_GM, Console::No },
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
