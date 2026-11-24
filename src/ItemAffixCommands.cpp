#include "AscensionClientSpellRows.h"
#include "Chat.h"
#include "Creature.h"
#include "CommandScript.h"
#include "Item.h"
#include "ItemAffix.h"
#include "Player.h"
#include "SpellAuraEffects.h"
#include "SpellAuras.h"
#include "SpellInfo.h"
#include "SpellMgr.h"
#include "StringConvert.h"
#include "StringFormat.h"
#include <algorithm>
#include <cctype>
#include <limits>
#include <optional>

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

// `.affix row` field names (Spell.dbc columns, DBCStructure.h SpellEntry); fieldN names any column.
struct RowFieldName
{
    char const* name;
    uint32 field;
};

constexpr RowFieldName RowFields[] = {
    { "CastingTimeIndex", 28 }, { "RecoveryTime", 29 }, { "CategoryRecoveryTime", 30 },
    { "DurationIndex", 40 }, { "ManaCost", 42 }, { "RangeIndex", 46 },
    { "EffectBasePoints0", 80 }, { "EffectBasePoints1", 81 }, { "EffectBasePoints2", 82 },
    { "EffectRadiusIndex0", 92 }, { "EffectRadiusIndex1", 93 }, { "EffectRadiusIndex2", 94 },
    { "EffectChainTarget0", 104 }, { "EffectChainTarget1", 105 }, { "EffectChainTarget2", 106 },
    { "ManaCostPercentage", 204 },
};

bool SameText(std::string_view a, std::string_view b)
{
    return a.size() == b.size() && std::equal(a.begin(), a.end(), b.begin(),
        [](char x, char y) { return std::tolower(uint8(x)) == std::tolower(uint8(y)); });
}

std::optional<uint32> RowField(std::string_view name)
{
    for (RowFieldName const& known : RowFields)
        if (SameText(name, known.name))
            return known.field;
    if (name.size() > 5 && SameText(name.substr(0, 5), "field"))
        if (Optional<uint32> field = Acore::StringTo<uint32>(name.substr(5)); field && AscensionSpellRow::Slot(*field))
            return *field;
    return std::nullopt;
}

std::string RowFieldLabel(uint32 field)
{
    for (RowFieldName const& known : RowFields)
        if (known.field == field)
            return known.name;
    return Acore::StringFormat("field{}", field);
}

struct RowEdit
{
    std::vector<std::pair<uint32, int64>> values;
    std::optional<std::string> description;
    std::optional<std::string> descriptionSuffix;
    std::optional<std::string> tooltip;
};

// key=value pairs; a value in double quotes may hold spaces. desc= replaces the Description,
// desc+= appends to it, tooltip= replaces the aura ToolTip.
bool ParseRowEdit(std::string_view text, RowEdit& edit, std::string& error)
{
    size_t pos = 0;
    while (pos < text.size())
    {
        if (std::isspace(uint8(text[pos])))
        {
            ++pos;
            continue;
        }
        size_t const eq = text.find('=', pos);
        if (eq == std::string_view::npos)
        {
            error = Acore::StringFormat("'{}' is not key=value", text.substr(pos));
            return false;
        }
        std::string_view key = text.substr(pos, eq - pos);
        bool append = false;
        if (!key.empty() && key.back() == '+')
        {
            append = true;
            key.remove_suffix(1);
        }
        pos = eq + 1;
        std::string value;
        if (pos < text.size() && text[pos] == '"')
        {
            size_t const close = text.find('"', pos + 1);
            if (close == std::string_view::npos)
            {
                error = Acore::StringFormat("missing closing quote after {}=", key);
                return false;
            }
            value = std::string(text.substr(pos + 1, close - pos - 1));
            pos = close + 1;
        }
        else
        {
            size_t end = pos;
            while (end < text.size() && !std::isspace(uint8(text[end])))
                ++end;
            value = std::string(text.substr(pos, end - pos));
            pos = end;
        }

        if (SameText(key, "desc"))
            (append ? edit.descriptionSuffix : edit.description) = value;
        else if (SameText(key, "tooltip") && !append)
            edit.tooltip = value;
        else if (std::optional<uint32> field = RowField(key); field && !append)
        {
            Optional<int64> number = Acore::StringTo<int64>(value);
            if (!number || *number < std::numeric_limits<int32>::min() || *number > std::numeric_limits<uint32>::max())
            {
                error = Acore::StringFormat("{}={} is not a 32-bit number", key, value);
                return false;
            }
            edit.values.emplace_back(*field, *number);
        }
        else
        {
            error = Acore::StringFormat("unknown field '{}'", key);
            return false;
        }
    }
    return true;
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
            { "summons", HandleSummonsCommand, SEC_GAMEMASTER,    Console::No },
            { "reroll",  HandleRerollCommand, SEC_ADMINISTRATOR, Console::No },
            { "give",    HandleGiveCommand,   SEC_ADMINISTRATOR, Console::No },
            { "row",     HandleRowCommand,    SEC_ADMINISTRATOR, Console::No },
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
        handler->PSendSysMessage("|cffFFFF00[ItemAffixes]|r summon affixes {} for this spec "
            "(ItemAffixes.SummonAffixes.Specs).", sItemAffixMgr->IsSummonSpec(profile.specId) ? "roll" : "do not roll");
        return true;
    }

    // .affix summons -- capped summon affix totals and the carrier of the selected creature.
    static bool HandleSummonsCommand(ChatHandler* handler)
    {
        Player* player = handler->GetSession()->GetPlayer();
        SummonStatTotals const stats = sItemAffixMgr->GetSummonStats(player);
        handler->PSendSysMessage("|cffFFFF00[ItemAffixes]|r summons: +{}% damage, +{}% health, +{}% attack speed.",
            stats.damagePct, stats.healthPct, stats.attackSpeedPct);

        Creature* target = handler->getSelectedCreature();
        if (!target)
            return true;
        Player* owner = ItemAffixMgr::GetSummonOwner(target);
        Aura* carrier = target->GetAura(SUMMON_AFFIX_CARRIER_SPELL);
        auto amount = [carrier](uint8 index)
        {
            AuraEffect const* effect = carrier ? carrier->GetEffect(index) : nullptr;
            return effect ? effect->GetAmount() : 0;
        };
        handler->PSendSysMessage("  {} (entry {}): owner {}, carrier {} (health {}%, attack speed {}%), max health {}.",
            target->GetName(), target->GetEntry(), owner ? owner->GetName() : "none", carrier ? "present" : "absent",
            amount(EFFECT_0), amount(EFFECT_1), target->GetMaxHealth());
        return true;
    }

    // .affix skills -- skill affixes on equipped items, the spell modifiers they produce and the
    // streamed Spell rows.
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
            SpellInfo const* info = sSpellMgr->GetSpellInfo(mod->targetSpellId);
            handler->PSendSysMessage("  mod op {} {} {} spell {} {} owner {}", uint32(mod->op),
                mod->type == SPELLMOD_PCT ? "pct" : "flat", mod->value, mod->targetSpellId,
                info ? info->SpellName[0] : "?", mod->ownerAura == carrier ? "carrier" : "STALE");
        }
        for (auto const& [spellId, effects] : data->rowEffects)
        {
            std::string ops;
            for (uint8 op = 0; op < SKILL_ROW_MAX_OPS; ++op)
            {
                if (effects.flat[op])
                    ops += Acore::StringFormat(" op{}{:+}", op, effects.flat[op]);
                if (effects.pct[op])
                    ops += Acore::StringFormat(" op{}{:+}%", op, effects.pct[op]);
            }
            handler->PSendSysMessage("  row spell {}:{} notes {} {}", spellId, ops, effects.notes.size(),
                data->sentRows.count(spellId) ? "sent" : "not sent");
        }
        return true;
    }

    // .affix give <equipSlot> <affix id|key> [crit] -- put one affix on an equipped item (testing; no class,
    // spec or known-spell check). A skill affix replaces the item's skill line; a stat affix (top of its
    // range) replaces the summon line for summon stats, else the first stat line; otherwise a line is added.
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
        if (!def)
        {
            handler->PSendSysMessage("|cffFFFF00[ItemAffixes]|r Unknown affix '{}'.", affix);
            return false;
        }
        Player* player = handler->GetSession()->GetPlayer();
        if (!def->IsSkill())
        {
            if (!sItemAffixMgr->GiveStatAffix(player, item, *def))
            {
                handler->SendSysMessage("|cffFFFF00[ItemAffixes]|r That item cannot carry affixes.");
                return false;
            }
            handler->PSendSysMessage("|cffFFFF00[ItemAffixes]|r {} now has [{}] {}.", item->GetTemplate()->Name1,
                def->id, def->name);
            return true;
        }
        if (!sItemAffixMgr->GiveSkillAffix(player, item, *def, crit && *crit == "crit"))
        {
            handler->SendSysMessage("|cffFFFF00[ItemAffixes]|r That item cannot carry affixes.");
            return false;
        }
        handler->PSendSysMessage("|cffFFFF00[ItemAffixes]|r {} now has [{}] {}{}.", item->GetTemplate()->Name1, def->id,
            def->key, sItemAffixMgr->PlayerKnowsSkillAffix(player, *def) ? "" : " (inactive: spell not known)");
        return true;
    }

    // .affix row <spellId> [field=value ...] [desc="..."] [desc+="..."] [tooltip="..."] | reset
    // Sends this player one Spell row (SMSG_PATCH_SPELL) to test what the client follows mid-session.
    // reset sends the row the server normally sends (with the player's streamed affixes).
    static bool HandleRowCommand(ChatHandler* handler, uint32 spellId, Tail args)
    {
        Player* player = handler->GetSession()->GetPlayer();
        std::optional<AscensionSpellRow> const original = AscensionOriginalSpellRow(spellId);
        if (!original)
        {
            handler->PSendSysMessage("|cffFFFF00[ItemAffixes]|r No client Spell row for {} (needs CoA.SendDisplayPatches = 1).",
                spellId);
            return false;
        }

        std::string_view text = args;
        while (!text.empty() && std::isspace(uint8(text.front())))
            text.remove_prefix(1);
        while (!text.empty() && std::isspace(uint8(text.back())))
            text.remove_suffix(1);

        if (text.empty() || SameText(text, "reset"))
        {
            bool const sent = AscensionSendPlayerSpellRow(player, spellId);
            handler->PSendSysMessage("|cffFFFF00[ItemAffixes]|r {} server row of {} {}.", sent ? "Sent" : "Could not send",
                spellId, original->Strings[AscensionSpellRow::NAME]);
            return sent;
        }

        RowEdit edit;
        std::string error;
        if (!ParseRowEdit(text, edit, error))
        {
            handler->PSendSysMessage("|cffFFFF00[ItemAffixes]|r {}.", error);
            return false;
        }

        AscensionSpellRow shown;
        bool const sent = AscensionSendPlayerSpellRow(player, spellId, [&](AscensionSpellRow& row)
        {
            for (auto const& [field, value] : edit.values)
                row.Set(field, uint32(value));
            if (edit.description)
                row.Strings[AscensionSpellRow::DESCRIPTION] = *edit.description;
            if (edit.descriptionSuffix)
                row.Strings[AscensionSpellRow::DESCRIPTION] += *edit.descriptionSuffix;
            if (edit.tooltip)
                row.Strings[AscensionSpellRow::TOOLTIP] = *edit.tooltip;
            shown = row;
        });
        if (!sent)
        {
            handler->SendSysMessage("|cffFFFF00[ItemAffixes]|r Could not send the row.");
            return false;
        }

        handler->PSendSysMessage("|cffFFFF00[ItemAffixes]|r Sent edited row of {} {}:", spellId,
            original->Strings[AscensionSpellRow::NAME]);
        for (auto const& [field, value] : edit.values)
            handler->PSendSysMessage("  {}: {} -> {}", RowFieldLabel(field), int32(original->Get(field)),
                int32(shown.Get(field)));
        if (edit.description || edit.descriptionSuffix)
            handler->PSendSysMessage("  Description: {}", shown.Strings[AscensionSpellRow::DESCRIPTION]);
        if (edit.tooltip)
            handler->PSendSysMessage("  ToolTip: {}", shown.Strings[AscensionSpellRow::TOOLTIP]);
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
