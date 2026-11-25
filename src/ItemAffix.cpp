#include "ItemAffix.h"

#include "AscensionClientSpellRows.h"
#include "Bag.h"
#include "CellImpl.h"
#include "Config.h"
#include "DBCStores.h"
#include "DatabaseEnv.h"
#include "GridNotifiers.h"
#include "GridNotifiersImpl.h"
#include "Item.h"
#include "ItemTemplate.h"
#include "Log.h"
#include "ObjectAccessor.h"
#include "ObjectGuid.h"
#include "ObjectMgr.h"
#include "Opcodes.h"
#include "Player.h"
#include "Random.h"
#include "SpellAuraEffects.h"
#include "SpellAuras.h"
#include "SpellInfo.h"
#include "SpellMgr.h"
#include "StringConvert.h"
#include "StringFormat.h"
#include "Tokenize.h"
#include "TradeData.h"
#include "WorldPacket.h"
#include "WorldSession.h"
#include <algorithm>
#include <cmath>
#include <functional>
#include <limits>
#include <map>
#include <tuple>
#include <unordered_set>

namespace
{
constexpr char const* ADDON_PREFIX = "AFXM";
constexpr uint8 MAX_AFFIX_SLOTS = 6;
constexpr float PEEK_UNIT_MAX_DISTANCE = 100.0f;
constexpr uint8 FIRST_COA_CLASS = 12;
constexpr uint8 LAST_COA_CLASS = 32;
constexpr size_t MAX_SKILL_TEXT = 60;
constexpr Milliseconds CARRIER_RESTORE_DELAY = 500ms;
constexpr Milliseconds STATUS_REFRESH_DELAY = 1000ms;
constexpr Milliseconds SUMMON_CARRIER_DELAY = 100ms;
constexpr float SUMMON_REFRESH_RANGE = 250.0f;
// Removed 2026-11-24 (rows are always streamed); only warned about when still set.
constexpr char const* REMOVED_STREAM_SPELL_ROWS_KEY = "ItemAffixes.SkillAffixes.StreamSpellRows";

uint8 GetItemCategory(ItemTemplate const* proto)
{
    switch (proto->InventoryType)
    {
        case INVTYPE_WEAPON:
        case INVTYPE_WEAPONMAINHAND:
        case INVTYPE_WEAPONOFFHAND:
        case INVTYPE_SHIELD:
            return ITEM_CAT_WEAPON_1H;
        case INVTYPE_2HWEAPON:
        case INVTYPE_RANGED:
            return ITEM_CAT_WEAPON_2H;
        case INVTYPE_RANGEDRIGHT:
            return ITEM_CAT_WAND;
        case INVTYPE_NECK:
        case INVTYPE_FINGER:
        case INVTYPE_TRINKET:
        case INVTYPE_RELIC:
            return ITEM_CAT_JEWELRY;
        case INVTYPE_FEET:
            return ITEM_CAT_BOOTS;
        default:
            return ITEM_CAT_ARMOR;
    }
}

bool ItemMatchesCategory(uint8 itemCat, uint8 required)
{
    switch (required)
    {
        case ITEM_CAT_ANY:    return true;
        case ITEM_CAT_WEAPON: return itemCat == ITEM_CAT_WEAPON_1H || itemCat == ITEM_CAT_WEAPON_2H;
        case ITEM_CAT_ARMOR:  return itemCat == ITEM_CAT_ARMOR || itemCat == ITEM_CAT_BOOTS;
        default:              return itemCat == required;
    }
}

bool Is2HWeapon(ItemTemplate const* proto)
{
    return proto->InventoryType == INVTYPE_2HWEAPON;
}

bool IsEquipped(Item const* item)
{
    return item->GetBagSlot() == INVENTORY_SLOT_BAG_0 && item->GetSlot() < EQUIPMENT_SLOT_END;
}

// Equipment: bag 255, 1-based slot. Backpack: bag 0. Bags 1-4. Matches the addon's lookup.
std::pair<uint8, uint8> GetLuaBagSlot(Item const* item)
{
    uint8 bagSlot  = item->GetBagSlot();
    uint8 itemSlot = item->GetSlot();
    if (bagSlot == INVENTORY_SLOT_BAG_0)
    {
        if (itemSlot < INVENTORY_SLOT_BAG_START)
            return { 255, uint8(itemSlot + 1) };
        return { 0, uint8(itemSlot - INVENTORY_SLOT_ITEM_START + 1) };
    }
    return { uint8(bagSlot - INVENTORY_SLOT_BAG_START + 1), uint8(itemSlot + 1) };
}

Item* GetItemByLuaBagSlot(Player* player, uint8 luaBag, uint8 luaSlot)
{
    if (luaSlot == 0)
        return nullptr;
    if (luaBag == 255)
    {
        if (luaSlot > EQUIPMENT_SLOT_END)
            return nullptr;
        return player->GetItemByPos(INVENTORY_SLOT_BAG_0, uint8(luaSlot - 1));
    }
    if (luaBag == 0)
    {
        uint8 cppSlot = uint8(INVENTORY_SLOT_ITEM_START + luaSlot - 1);
        if (cppSlot >= INVENTORY_SLOT_ITEM_END)
            return nullptr;
        return player->GetItemByPos(INVENTORY_SLOT_BAG_0, cppSlot);
    }
    uint8 cppBag = uint8(INVENTORY_SLOT_BAG_START + luaBag - 1);
    if (cppBag >= INVENTORY_SLOT_BAG_END)
        return nullptr;
    Bag* bag = player->GetBagByPos(cppBag);
    if (!bag || luaSlot > bag->GetBagSize())
        return nullptr;
    return bag->GetItemByPos(luaSlot - 1);
}

// Piecewise linear approximation of Blizzard's item stat budget by item level.
float ComputeItemBudget(uint32 itemLevel)
{
    float ilvl = float(itemLevel);
    if (itemLevel <= 66)
        return ilvl * 0.78f + 1.5f;
    if (itemLevel <= 114)
        return ilvl * 1.25f - 28.5f;
    return ilvl * 1.92f - 105.0f;
}

float GetSlotBudgetMod(uint32 inventoryType)
{
    switch (inventoryType)
    {
        case INVTYPE_HEAD:
        case INVTYPE_CHEST:
        case INVTYPE_ROBE:
        case INVTYPE_LEGS:
        case INVTYPE_2HWEAPON:
        case INVTYPE_RANGED:
            return 1.00f;
        case INVTYPE_SHOULDERS:
        case INVTYPE_WAIST:
        case INVTYPE_FEET:
        case INVTYPE_HANDS:
            return 0.74f;
        default:
            return 0.54f;
    }
}

char const* const StatNames[GSTAT_MAX] = {
    "Stamina", "Strength", "Agility", "Intellect", "Spirit",
    "Attack Power", "Ranged Attack Power", "Spell Power", "Mp5",
    "Armor", "Crit Rating", "Haste Rating", "Hit Rating",
    "Dodge Rating", "Defense Rating", "Parry Rating", "Expertise Rating",
};

char const* GetStatName(uint8 statOp)
{
    switch (statOp)
    {
        case GSTAT_SUMMON_HEALTH_PCT:       return "Summon Health";
        case GSTAT_SUMMON_DAMAGE_PCT:       return "Summon Damage";
        case GSTAT_SUMMON_ATTACK_SPEED_PCT: return "Summon Attack Speed";
        default:                            return statOp < GSTAT_MAX ? StatNames[statOp] : "Unknown";
    }
}

std::string StatText(uint8 statOp, int32 minVal, int32 maxVal)
{
    char const* unit = IsSummonStat(statOp) ? "%" : "";
    if (minVal == maxVal)
        return Acore::StringFormat("+{}{} {}", minVal, unit, GetStatName(statOp));
    return Acore::StringFormat("+{}-{}{} {}", minVal, maxVal, unit, GetStatName(statOp));
}

// Item budget cost per point (WotLK exchange rates).
float GetStatCost(uint8 statOp)
{
    switch (statOp)
    {
        case GSTAT_ATTACK_POWER:
        case GSTAT_RANGED_AP:
            return 0.5f;
        case GSTAT_SPELL_POWER:
            return 0.86f;
        case GSTAT_ARMOR:
            return 0.1f;  // armor is cheap: 10 armor per budget point
        case GSTAT_SUMMON_HEALTH_PCT:
            return 0.75f;
        case GSTAT_SUMMON_ATTACK_SPEED_PCT:
            return 1.5f;
        default:
            return 1.0f;
    }
}

// The ItemMod a primary-attribute affix corresponds to; 0 for every other stat.
uint8 AttributeItemMod(uint8 statOp)
{
    switch (statOp)
    {
        case GSTAT_STRENGTH:  return ITEM_MOD_STRENGTH;
        case GSTAT_AGILITY:   return ITEM_MOD_AGILITY;
        case GSTAT_INTELLECT: return ITEM_MOD_INTELLECT;
        case GSTAT_SPIRIT:    return ITEM_MOD_SPIRIT;
        default:              return 0;
    }
}

struct ValueRange { int32 minVal; int32 maxVal; };

ValueRange BudgetRange(uint8 statOp, float budget, float minRoll)
{
    float cost = GetStatCost(statOp);
    int32 maxVal = std::max(1, int32(std::floor(budget / cost)));
    int32 minVal = std::clamp(int32(std::floor(budget * minRoll / cost)), 1, maxVal);
    return { minVal, maxVal };
}

int32 Boost(int32 value) { return (value * 3 + 1) / 2; }

void ApplyGenericStat(Player* player, uint8 statOp, int32 value, bool apply)
{
    float fval = float(value);
    switch (statOp)
    {
        case GSTAT_STAMINA:   player->HandleStatFlatModifier(UNIT_MOD_STAT_STAMINA,   TOTAL_VALUE, fval, apply); break;
        case GSTAT_STRENGTH:  player->HandleStatFlatModifier(UNIT_MOD_STAT_STRENGTH,  TOTAL_VALUE, fval, apply); break;
        case GSTAT_AGILITY:   player->HandleStatFlatModifier(UNIT_MOD_STAT_AGILITY,   TOTAL_VALUE, fval, apply); break;
        case GSTAT_INTELLECT: player->HandleStatFlatModifier(UNIT_MOD_STAT_INTELLECT, TOTAL_VALUE, fval, apply); break;
        case GSTAT_SPIRIT:    player->HandleStatFlatModifier(UNIT_MOD_STAT_SPIRIT,    TOTAL_VALUE, fval, apply); break;
        case GSTAT_ATTACK_POWER:
            player->HandleStatFlatModifier(UNIT_MOD_ATTACK_POWER, TOTAL_VALUE, fval, apply);
            break;
        case GSTAT_RANGED_AP:
            player->HandleStatFlatModifier(UNIT_MOD_ATTACK_POWER_RANGED, TOTAL_VALUE, fval, apply);
            break;
        case GSTAT_SPELL_POWER: player->ApplySpellPowerBonus(value, apply); break;
        case GSTAT_MP5:         player->ApplyManaRegenBonus(value, apply); break;
        case GSTAT_ARMOR:       player->HandleStatFlatModifier(UNIT_MOD_ARMOR, TOTAL_VALUE, fval, apply); break;
        case GSTAT_CRIT_RATING:
            player->ApplyRatingMod(CR_CRIT_MELEE,  value, apply);
            player->ApplyRatingMod(CR_CRIT_RANGED, value, apply);
            player->ApplyRatingMod(CR_CRIT_SPELL,  value, apply);
            break;
        case GSTAT_HASTE_RATING:
            player->ApplyRatingMod(CR_HASTE_MELEE,  value, apply);
            player->ApplyRatingMod(CR_HASTE_RANGED, value, apply);
            player->ApplyRatingMod(CR_HASTE_SPELL,  value, apply);
            break;
        case GSTAT_HIT_RATING:
            player->ApplyRatingMod(CR_HIT_MELEE,  value, apply);
            player->ApplyRatingMod(CR_HIT_RANGED, value, apply);
            player->ApplyRatingMod(CR_HIT_SPELL,  value, apply);
            break;
        case GSTAT_DODGE_RATING:     player->ApplyRatingMod(CR_DODGE,         value, apply); break;
        case GSTAT_DEFENSE_RATING:   player->ApplyRatingMod(CR_DEFENSE_SKILL, value, apply); break;
        case GSTAT_PARRY_RATING:     player->ApplyRatingMod(CR_PARRY,         value, apply); break;
        case GSTAT_EXPERTISE_RATING: player->ApplyRatingMod(CR_EXPERTISE,     value, apply); break;
        case GSTAT_SUMMON_HEALTH_PCT:
        case GSTAT_SUMMON_DAMAGE_PCT:
        case GSTAT_SUMMON_ATTACK_SPEED_PCT:
            sItemAffixMgr->ApplySummonStat(player, statOp, value, apply);
            break;
        default:
            LOG_ERROR("module", "mod-item-affixes: unknown stat_op {}", statOp);
            break;
    }
}

std::string SerializeOptions(std::vector<ReforgeOption> const& opts)
{
    std::string out;
    for (size_t i = 0; i < opts.size(); ++i)
    {
        if (i)
            out += ',';
        out += Acore::StringFormat("{}:{}:{}", opts[i].affixId, opts[i].rolledValue, opts[i].isCrit ? 1 : 0);
    }
    return out;
}

std::vector<ReforgeOption> ParseOptions(std::string const& raw)
{
    std::vector<ReforgeOption> opts;
    for (std::string_view part : Acore::Tokenize(raw, ',', false))
    {
        std::vector<std::string_view> fields = Acore::Tokenize(part, ':', false);
        if (fields.size() != 3)
            continue;
        Optional<uint32> id = Acore::StringTo<uint32>(fields[0]);
        Optional<int32> value = Acore::StringTo<int32>(fields[1]);
        if (!id || !value)
            continue;
        opts.push_back({ *id, *value, fields[2] == "1" });
    }
    return opts;
}

int32 ScaleSkillValue(int32 value, int32 scalePct)
{
    return int32(std::lround(double(value) * scalePct / 100.0));
}

bool IsMillisecondOp(uint8 op)
{
    return op == SPELLMOD_COOLDOWN || op == SPELLMOD_CASTING_TIME || op == SPELLMOD_DURATION;
}

// Same number the review names carry (tools/skill_affixes/merge_reviews.py display_number).
std::string SkillDisplayNumber(SkillAffixEffect const& fx, int32 value)
{
    uint32 v = uint32(std::abs(value));
    if (!fx.pct && IsMillisecondOp(fx.op))
    {
        if (v % 1000 == 0)
            return Acore::StringFormat("{}", v / 1000);
        std::string out = Acore::StringFormat("{:.2f}", v / 1000.0);
        while (!out.empty() && out.back() == '0')
            out.pop_back();
        if (!out.empty() && out.back() == '.')
            out.pop_back();
        return out;
    }
    return Acore::StringFormat("{}", v);
}

uint32 SpellCooldownBase(SpellInfo const* info)
{
    return info ? std::max(info->RecoveryTime, info->CategoryRecoveryTime) : 0;
}

// Technical floor only (no balance cap): summed cooldown, cast time and cost cuts never go below zero.
int32 ClampSkillValue(uint8 op, bool pct, int32 value, SpellInfo const* reference)
{
    if (op != SPELLMOD_COOLDOWN && op != SPELLMOD_CASTING_TIME && op != SPELLMOD_COST)
        return value;
    if (pct)
        return std::max(value, -100);
    int32 base = 0;
    if (reference)
    {
        if (op == SPELLMOD_COOLDOWN)
            base = int32(SpellCooldownBase(reference));
        else if (op == SPELLMOD_CASTING_TIME)
            base = reference->CastTimeEntry ? std::max(reference->CastTimeEntry->CastTime, 0) : 0;
        else
            base = int32(reference->ManaCost);
    }
    return std::max(value, -base);
}

template <typename Fn>
void ForEachCarriedItem(Player* player, Fn&& fn)
{
    for (uint8 slot = EQUIPMENT_SLOT_START; slot < INVENTORY_SLOT_ITEM_END; ++slot)
        if (slot < INVENTORY_SLOT_BAG_START || slot >= INVENTORY_SLOT_ITEM_START)
            if (Item* item = player->GetItemByPos(INVENTORY_SLOT_BAG_0, slot))
                fn(item);

    for (uint8 bagSlot = INVENTORY_SLOT_BAG_START; bagSlot < INVENTORY_SLOT_BAG_END; ++bagSlot)
        if (Bag* bag = player->GetBagByPos(bagSlot))
            for (uint32 i = 0; i < bag->GetBagSize(); ++i)
                if (Item* item = bag->GetItemByPos(i))
                    fn(item);
}
}

// ---------------------------------------------------------------------------
// Streamed Spell rows
// ---------------------------------------------------------------------------

namespace
{
// Spell.dbc columns (DBCStructure.h SpellEntry).
constexpr uint32 ROW_CASTING_TIME_INDEX     = 28;
constexpr uint32 ROW_RECOVERY_TIME          = 29;
constexpr uint32 ROW_CATEGORY_RECOVERY_TIME = 30;
constexpr uint32 ROW_DURATION_INDEX         = 40;
constexpr uint32 ROW_MANA_COST              = 42;
constexpr uint32 ROW_RANGE_INDEX            = 46;
constexpr uint32 ROW_EFFECT                 = 71;
constexpr uint32 ROW_EFFECT_DIE_SIDES       = 74;
constexpr uint32 ROW_EFFECT_BASE_POINTS     = 80;
constexpr uint32 ROW_EFFECT_RADIUS_INDEX    = 92;
constexpr uint32 ROW_EFFECT_CHAIN_TARGET    = 104;
constexpr uint32 ROW_MANA_COST_PERCENTAGE   = 204;
constexpr size_t MAX_ROW_NOTE               = 400;
constexpr Milliseconds SPELL_ROW_RESET_DELAY = 5000ms;
static_assert(SKILL_ROW_MAX_OPS >= MAX_SPELLMOD);

bool HasRowField(uint8 op)
{
    switch (op)
    {
        case SPELLMOD_COOLDOWN:
        case SPELLMOD_COST:
        case SPELLMOD_EFFECT1:
        case SPELLMOD_EFFECT2:
        case SPELLMOD_EFFECT3:
        case SPELLMOD_ALL_EFFECTS:
        case SPELLMOD_JUMP_TARGETS:
        case SPELLMOD_CASTING_TIME:
        case SPELLMOD_RANGE:
        case SPELLMOD_DURATION:
        case SPELLMOD_RADIUS:
            return true;
        default:
            return false;
    }
}

bool Touches(SkillRowEffects const& fx, uint8 op)
{
    return fx.flat[op] || fx.pct[op];
}

// Same order as Player::ApplySpellMod.
double Modified(double base, SkillRowEffects const& fx, uint8 op)
{
    double const mul = (100.0 + fx.pct[op]) / 100.0;
    if (op == SPELLMOD_CASTING_TIME || op == SPELLMOD_DURATION)
        return base + fx.flat[op] > 0 ? (base + fx.flat[op]) * mul : 0.0;
    return base * mul + fx.flat[op];
}

uint32 NonNegative(double value)
{
    return value <= 0.0 ? 0 : uint32(std::min<double>(std::lround(value), std::numeric_limits<int32>::max()));
}

// Nearest key, the lower one on a tie (tools/skill_affixes/analyze_index_rounding.py).
template <typename Key>
uint32 NearestIndex(std::map<Key, uint32> const& rows, double want)
{
    if (rows.empty())
        return 0;
    auto upper = rows.lower_bound(Key(std::ceil(want)));
    if (upper == rows.end())
        return std::prev(upper)->second;
    if (upper == rows.begin())
        return upper->second;
    auto lower = std::prev(upper);
    return want - double(lower->first) <= double(upper->first) - want ? lower->second : upper->second;
}

uint32 NearestCastTimeIndex(double want)
{
    static std::map<int32, uint32> const rows = []
    {
        std::map<int32, uint32> found;
        for (uint32 id = 1; id < sSpellCastTimesStore.GetNumRows(); ++id)
            if (SpellCastTimesEntry const* entry = sSpellCastTimesStore.LookupEntry(id))
                if (entry->CastTime >= 0)
                    found.emplace(entry->CastTime, id);
        return found;
    }();
    return NearestIndex(rows, want);
}

uint32 NearestDurationIndex(double want)
{
    static std::map<int32, uint32> const rows = []
    {
        std::map<int32, uint32> found;
        for (uint32 id = 1; id < sSpellDurationStore.GetNumRows(); ++id)
            if (SpellDurationEntry const* entry = sSpellDurationStore.LookupEntry(id))
                if (entry->Duration[0] > 0 && !entry->Duration[1] && entry->Duration[2] == entry->Duration[0])
                    found.emplace(entry->Duration[0], id);
        return found;
    }();
    return NearestIndex(rows, want);
}

// Radius in thousandths of a yard.
uint32 NearestRadiusIndex(double want)
{
    static std::map<int32, uint32> const rows = []
    {
        std::map<int32, uint32> found;
        for (uint32 id = 1; id < sSpellRadiusStore.GetNumRows(); ++id)
            if (SpellRadiusEntry const* entry = sSpellRadiusStore.LookupEntry(id))
                if (entry->RadiusMin > 0.0f && entry->RadiusPerLevel == 0.0f)
                    found.emplace(int32(std::lround(entry->RadiusMin * 1000.0f)), id);
        return found;
    }();
    return NearestIndex(rows, want * 1000.0);
}

// The longest range row not beyond the server's range, with the same minimum ranges and flags, so the
// client never accepts a cast the server rejects as out of range.
uint32 RangeIndexAtMost(SpellRangeEntry const* base, double hostile, double friendly)
{
    uint32 best = 0;
    float bestHostile = -1.0f;
    for (uint32 id = 1; id < sSpellRangeStore.GetNumRows(); ++id)
    {
        SpellRangeEntry const* entry = sSpellRangeStore.LookupEntry(id);
        if (!entry || entry->Flags != base->Flags || entry->RangeMin[0] != base->RangeMin[0] ||
            entry->RangeMin[1] != base->RangeMin[1])
            continue;
        if (entry->RangeMax[0] > hostile + 0.001 || entry->RangeMax[1] > friendly + 0.001)
            continue;
        if (entry->RangeMax[0] > bestHostile)
        {
            best = id;
            bestHostile = entry->RangeMax[0];
        }
    }
    return best;
}

size_t RowSignature(SkillRowEffects const& fx)
{
    size_t seed = 0;
    auto mix = [&seed](size_t value) { seed ^= value + 0x9e3779b97f4a7c15ULL + (seed << 6) + (seed >> 2); };
    for (uint8 op = 0; op < SKILL_ROW_MAX_OPS; ++op)
    {
        mix(std::hash<int32>()(fx.flat[op]));
        mix(std::hash<int32>()(fx.pct[op]));
    }
    for (std::string const& note : fx.notes)
        mix(std::hash<std::string>()(note));
    return seed;
}

void SkillAffixSpellRowOverride(Player* player, AscensionSpellRow& row)
{
    if (!player || !sItemAffixMgr->Handles(player))
        return;
    ItemAffixPlayerData* data = player->CustomData.Get<ItemAffixPlayerData>("ItemAffixData");
    if (!data)
        return;
    if (auto it = data->rowEffects.find(row.SpellId()); it != data->rowEffects.end())
        ItemAffixMgr::ApplySpellRowEffects(it->second, row);
}
}

void ItemAffixMgr::ApplySpellRowEffects(SkillRowEffects const& fx, AscensionSpellRow& row)
{
    if (Touches(fx, SPELLMOD_COOLDOWN))
        for (uint32 field : { ROW_RECOVERY_TIME, ROW_CATEGORY_RECOVERY_TIME })
            if (uint32 base = row.Get(field))
                row.Set(field, NonNegative(Modified(base, fx, SPELLMOD_COOLDOWN)));

    if (Touches(fx, SPELLMOD_COST))
    {
        if (uint32 base = row.Get(ROW_MANA_COST))
            row.Set(ROW_MANA_COST, NonNegative(Modified(base, fx, SPELLMOD_COST)));
        if (uint32 base = row.Get(ROW_MANA_COST_PERCENTAGE); base && fx.pct[SPELLMOD_COST])
            row.Set(ROW_MANA_COST_PERCENTAGE, NonNegative(base * (100.0 + fx.pct[SPELLMOD_COST]) / 100.0));
    }

    static constexpr uint8 EffectOps[MAX_SPELL_EFFECTS] = { SPELLMOD_EFFECT1, SPELLMOD_EFFECT2, SPELLMOD_EFFECT3 };
    for (uint8 i = 0; i < MAX_SPELL_EFFECTS; ++i)
    {
        if (!row.Get(ROW_EFFECT + i))
            continue;
        if (Touches(fx, SPELLMOD_ALL_EFFECTS) || Touches(fx, EffectOps[i]))
        {
            // DBC base points are the shown value minus one when the effect rolls dice.
            int32 const offset = int32(row.Get(ROW_EFFECT_DIE_SIDES + i)) > 0 ? 1 : 0;
            double value = double(int32(row.Get(ROW_EFFECT_BASE_POINTS + i)) + offset);
            if (Touches(fx, SPELLMOD_ALL_EFFECTS))
                value = Modified(value, fx, SPELLMOD_ALL_EFFECTS);
            if (Touches(fx, EffectOps[i]))
                value = Modified(value, fx, EffectOps[i]);
            row.Set(ROW_EFFECT_BASE_POINTS + i, uint32(int32(std::lround(value)) - offset));
        }
        if (uint32 chain = row.Get(ROW_EFFECT_CHAIN_TARGET + i); chain && Touches(fx, SPELLMOD_JUMP_TARGETS))
            row.Set(ROW_EFFECT_CHAIN_TARGET + i, NonNegative(Modified(chain, fx, SPELLMOD_JUMP_TARGETS)));
        if (Touches(fx, SPELLMOD_RADIUS))
            if (SpellRadiusEntry const* radius = sSpellRadiusStore.LookupEntry(row.Get(ROW_EFFECT_RADIUS_INDEX + i)))
                if (radius->RadiusMin > 0.0f)
                    if (uint32 index = NearestRadiusIndex(Modified(radius->RadiusMin, fx, SPELLMOD_RADIUS)))
                        row.Set(ROW_EFFECT_RADIUS_INDEX + i, index);
    }

    if (Touches(fx, SPELLMOD_CASTING_TIME))
        if (SpellCastTimesEntry const* cast = sSpellCastTimesStore.LookupEntry(row.Get(ROW_CASTING_TIME_INDEX)))
            if (cast->CastTime > 0)
                if (uint32 index = NearestCastTimeIndex(Modified(cast->CastTime, fx, SPELLMOD_CASTING_TIME)))
                    row.Set(ROW_CASTING_TIME_INDEX, index);

    if (Touches(fx, SPELLMOD_DURATION))
        if (SpellDurationEntry const* duration = sSpellDurationStore.LookupEntry(row.Get(ROW_DURATION_INDEX)))
            if (duration->Duration[0] > 0)
                if (uint32 index = NearestDurationIndex(Modified(duration->Duration[0], fx, SPELLMOD_DURATION)))
                    row.Set(ROW_DURATION_INDEX, index);

    if (Touches(fx, SPELLMOD_RANGE))
        if (SpellRangeEntry const* range = sSpellRangeStore.LookupEntry(row.Get(ROW_RANGE_INDEX)))
            if (range->RangeMax[0] > 0.0f)
                if (uint32 index = RangeIndexAtMost(range, Modified(range->RangeMax[0], fx, SPELLMOD_RANGE),
                        Modified(range->RangeMax[1], fx, SPELLMOD_RANGE)))
                    row.Set(ROW_RANGE_INDEX, index);

    if (!fx.notes.empty())
    {
        std::string note = "Affixes: ";
        for (size_t i = 0; i < fx.notes.size(); ++i)
            note += (i ? "; " : "") + fx.notes[i];
        if (note.size() > MAX_ROW_NOTE)
            note.resize(MAX_ROW_NOTE);
        std::string& description = row.Strings[AscensionSpellRow::DESCRIPTION];
        description += (description.empty() ? "" : "\n\n") + std::string("|cff1eff00") + note + "|r";
    }
}

void ItemAffixMgr::SyncSpellRows(Player* player, bool resetClassSpells)
{
    if (!player || !Handles(player))
        return;
    ItemAffixPlayerData* data = player->CustomData.GetDefault<ItemAffixPlayerData>("ItemAffixData");
    if (!data->rowsReady)
        return;

    std::unordered_set<uint32> originals;
    for (auto const& [spellId, signature] : data->sentRows)
        if (!data->rowEffects.count(spellId))
            originals.insert(spellId);
    if (resetClassSpells)
        if (auto pool = _skillPoolByClass.find(player->getClass()); pool != _skillPoolByClass.end())
            for (uint32 affixId : pool->second)
                if (AffixDefinition const* def = GetAffixDef(affixId))
                    for (uint32 spellId : def->targets)
                        if (!data->rowEffects.count(spellId))
                            originals.insert(spellId);

    for (uint32 spellId : originals)
    {
        AscensionSendPlayerSpellRow(player, spellId);
        data->sentRows.erase(spellId);
    }

    for (auto const& [spellId, effects] : data->rowEffects)
    {
        size_t const signature = RowSignature(effects);
        auto sent = data->sentRows.find(spellId);
        if (!resetClassSpells && sent != data->sentRows.end() && sent->second == signature)
            continue;
        if (AscensionSendPlayerSpellRow(player, spellId))
            data->sentRows[spellId] = signature;
    }
}

// Login and spec change: the client may still hold rows from before, so once the client is ready
// every spell the class's skill affixes can touch is sent again.
void ItemAffixMgr::ScheduleSpellRowReset(Player* player)
{
    if (!_skillAffixCount || !Handles(player))
        return;
    ItemAffixPlayerData* data = player->CustomData.GetDefault<ItemAffixPlayerData>("ItemAffixData");
    data->rowsReady = false;
    if (data->rowResetScheduled)
        return;
    data->rowResetScheduled = true;
    player->m_Events.AddEventAtOffset([player]()
    {
        ItemAffixPlayerData* pending = player->CustomData.Get<ItemAffixPlayerData>("ItemAffixData");
        if (!pending)
            return;
        pending->rowResetScheduled = false;
        pending->rowsReady = true;
        if (player->IsInWorld())
            sItemAffixMgr->SyncSpellRows(player, true);
    }, SPELL_ROW_RESET_DELAY);
}

struct ItemAffixMgr::RollContext
{
    Player*                 player = nullptr;
    ItemTemplate const*     proto = nullptr;
    AffixSpecProfile        profile;
    float                   budget = 0.0f;
    uint8                   itemCategory = ITEM_CAT_ANY;
    bool                    hasMana = false;
    bool                    summonSpec = false;
    bool                    hasSummonStat = false;  // at most one summon affix per item
    std::unordered_set<uint8> takenStats;  // stats already on the item; avoided while alternatives exist
};

ItemAffixPlayerData::~ItemAffixPlayerData()
{
    // Only reached with mods left when the Player is destroyed without a carrier removal or logout.
    for (SpellModifier* mod : skillMods)
        delete mod;
}

ItemAffixMgr* ItemAffixMgr::instance()
{
    static ItemAffixMgr inst;
    return &inst;
}

// ---------------------------------------------------------------------------
// Loading
// ---------------------------------------------------------------------------

void ItemAffixMgr::LoadConfig()
{
    auto slots = [](char const* key, uint8 def) { return std::min<uint8>(sConfigMgr->GetOption<uint8>(key, def), MAX_AFFIX_SLOTS); };

    _enabled             = sConfigMgr->GetOption<bool>("ItemAffixes.Enable", true);
    _excludeBots         = sConfigMgr->GetOption<bool>("ItemAffixes.ExcludeBots", true);
    _rollUnrolledOnEquip = sConfigMgr->GetOption<bool>("ItemAffixes.RollUnrolledOnEquip", true);

    _slotCount[ITEM_QUALITY_UNCOMMON] = slots("ItemAffixes.SlotCountGreen", 1);
    _slotCount[ITEM_QUALITY_RARE]     = slots("ItemAffixes.SlotCountBlue", 2);
    _slotCount[ITEM_QUALITY_EPIC]     = slots("ItemAffixes.SlotCountPurple", 3);
    _slotCountLegendary               = slots("ItemAffixes.SlotCountLegendary", 4);
    _twoHanderBonusSlots              = slots("ItemAffixes.TwoHanderBonusSlots", 1);
    _bonusSlotChance = std::min<uint32>(sConfigMgr->GetOption<uint32>("ItemAffixes.BonusSlotChance", 5), 100);

    _budgetFraction[ITEM_QUALITY_UNCOMMON] = sConfigMgr->GetOption<float>("ItemAffixes.BudgetFractionGreen", 0.18f);
    _budgetFraction[ITEM_QUALITY_RARE]     = sConfigMgr->GetOption<float>("ItemAffixes.BudgetFractionBlue", 0.13f);
    _budgetFraction[ITEM_QUALITY_EPIC]     = sConfigMgr->GetOption<float>("ItemAffixes.BudgetFractionPurple", 0.10f);
    _budgetFractionLegendary               = sConfigMgr->GetOption<float>("ItemAffixes.BudgetFractionLegendary", 0.09f);
    _budgetMinRoll       = std::clamp(sConfigMgr->GetOption<float>("ItemAffixes.BudgetMinRoll", 0.75f), 0.0f, 1.0f);
    _statMultiplier      = std::max(0.0f, sConfigMgr->GetOption<float>("ItemAffixes.StatMultiplier", 1.5f));
    _maxBudgetItemLevel  = sConfigMgr->GetOption<uint32>("ItemAffixes.MaxBudgetItemLevel", 100);
    _secondaryAttributeWeightPct = std::min<uint32>(sConfigMgr->GetOption<uint32>("ItemAffixes.SecondaryAttributeWeightPct", 33), 100);

    _critRollEnabled = sConfigMgr->GetOption<bool>("ItemAffixes.EnableCritRolls", true);
    _critRollChance  = std::min<uint32>(sConfigMgr->GetOption<uint32>("ItemAffixes.CritRollChance", 10), 100);

    _reforgeOptions[ITEM_QUALITY_UNCOMMON] = std::clamp<uint8>(sConfigMgr->GetOption<uint8>("ItemAffixes.ReforgeOptionsGreen", 1), 1, 6);
    _reforgeOptions[ITEM_QUALITY_RARE]     = std::clamp<uint8>(sConfigMgr->GetOption<uint8>("ItemAffixes.ReforgeOptionsBlue", 2), 1, 6);
    _reforgeOptions[ITEM_QUALITY_EPIC]     = std::clamp<uint8>(sConfigMgr->GetOption<uint8>("ItemAffixes.ReforgeOptionsPurple", 3), 1, 6);
    _reforgeOptionsLegendary               = std::clamp<uint8>(sConfigMgr->GetOption<uint8>("ItemAffixes.ReforgeOptionsLegendary", 4), 1, 6);
    _reforgeBaseCost[ITEM_QUALITY_UNCOMMON] = sConfigMgr->GetOption<uint32>("ItemAffixes.ReforgeBaseCostGreen", 5000);
    _reforgeBaseCost[ITEM_QUALITY_RARE]     = sConfigMgr->GetOption<uint32>("ItemAffixes.ReforgeBaseCostBlue", 20000);
    _reforgeBaseCost[ITEM_QUALITY_EPIC]     = sConfigMgr->GetOption<uint32>("ItemAffixes.ReforgeBaseCostPurple", 75000);
    _reforgeBaseCostLegendary               = sConfigMgr->GetOption<uint32>("ItemAffixes.ReforgeBaseCostLegendary", 250000);
    _reforgeReferenceItemLevel = sConfigMgr->GetOption<uint32>("ItemAffixes.ReforgeReferenceItemLevel", 60);
    _reforgeCostIncreasePct    = sConfigMgr->GetOption<uint32>("ItemAffixes.ReforgeCostIncreasePct", 50);

    _skillAffixesEnabled = sConfigMgr->GetOption<bool>("ItemAffixes.SkillAffixes.Enable", true);
    _skillAffixChance    = std::min<uint32>(sConfigMgr->GetOption<uint32>("ItemAffixes.SkillAffixChance", 20), 100);

    _summonAffixesEnabled = sConfigMgr->GetOption<bool>("ItemAffixes.SummonAffixes.Enable", true);
    _summonCapPct         = std::clamp<int32>(
        sConfigMgr->GetOption<int32>("ItemAffixes.SummonAffixes.CapPct", 100), 0, 1000);
    _summonSpecs.clear();
    std::string const specList =
        sConfigMgr->GetOption<std::string>("ItemAffixes.SummonAffixes.Specs", "11,17,35,36,50,70,81,90");
    for (std::string_view token : Acore::Tokenize(specList, ',', false))
    {
        size_t const first = token.find_first_not_of(' ');
        size_t const last = token.find_last_not_of(' ');
        std::string_view const trimmed =
            first == std::string_view::npos ? std::string_view() : token.substr(first, last - first + 1);
        if (Optional<uint32> spec = Acore::StringTo<uint32>(trimmed))
            _summonSpecs.insert(*spec);
        else if (!trimmed.empty())
            LOG_ERROR("module", "mod-item-affixes: ItemAffixes.SummonAffixes.Specs: '{}' is not a spec id.", trimmed);
    }
    for (std::string const& key : sConfigMgr->GetKeysByString(REMOVED_STREAM_SPELL_ROWS_KEY))
        if (key == REMOVED_STREAM_SPELL_ROWS_KEY)
            LOG_WARN("module", "mod-item-affixes: {} is no longer used and is ignored; skill affix Spell rows are "
                "always streamed. Remove it from the config.", REMOVED_STREAM_SPELL_ROWS_KEY);
    AscensionSetSpellRowOverride(&SkillAffixSpellRowOverride);
}

void ItemAffixMgr::LoadAffixTemplates()
{
    _defs.clear();
    _defsByKey.clear();
    _skillPoolByClass.clear();
    _skillAffixCount = 0;
    {
        std::lock_guard<std::mutex> guard(_cacheLock);
        _cache.clear();
    }

    if (!_enabled)
    {
        LOG_INFO("module", "mod-item-affixes: disabled (ItemAffixes.Enable = 0).");
        return;
    }

    // mod_item_affixes_template_skill.sql adds affix_type; without it the table holds stat rows only.
    bool const skillSchema = WorldDatabase.Query(
        "SELECT 1 FROM information_schema.COLUMNS WHERE TABLE_SCHEMA = DATABASE() "
        "AND TABLE_NAME = 'affix_template' AND COLUMN_NAME = 'affix_type'") != nullptr;

    QueryResult result = WorldDatabase.Query(
        "SELECT id, name, weight, stat_op, role_mask, item_category, min_quality, requires_mana "
        "FROM affix_template WHERE weight > 0{}", skillSchema ? " AND affix_type = 1" : "");
    if (result)
    {
        do
        {
            Field* f = result->Fetch();
            AffixDefinition def;
            def.id           = f[0].Get<uint32>();
            def.name         = f[1].Get<std::string>();
            def.weight       = f[2].Get<uint32>();
            def.statOp       = f[3].Get<uint8>();
            def.roleMask     = f[4].Get<uint8>();
            def.itemCategory = f[5].Get<uint8>();
            def.minQuality   = f[6].Get<uint8>();
            def.requiresMana = f[7].Get<uint8>() != 0;
            if (def.statOp >= GSTAT_MAX && !IsSummonStat(def.statOp))
            {
                LOG_ERROR("module", "mod-item-affixes: affix {} has unsupported stat_op {}, skipped.", def.id, def.statOp);
                continue;
            }
            _defs[def.id] = std::move(def);
        } while (result->NextRow());
    }
    LOG_INFO("module", "mod-item-affixes: loaded {} affix template(s).", _defs.size());
    if (_summonAffixesEnabled && !sSpellMgr->GetSpellInfo(SUMMON_AFFIX_CARRIER_SPELL))
        LOG_ERROR("module", "mod-item-affixes: summon carrier spell {} missing from spell_dbc; summon health and "
            "attack speed affixes have no effect.", SUMMON_AFFIX_CARRIER_SPELL);

    if (!_skillAffixesEnabled)
        LOG_INFO("module", "mod-item-affixes: skill affixes disabled (ItemAffixes.SkillAffixes.Enable = 0).");
    else if (!skillSchema)
        LOG_ERROR("module", "mod-item-affixes: affix_template has no affix_type column; apply "
            "mod_item_affixes_template_skill.sql. Skill affixes disabled.");
    else
        LoadSkillAffixes();

    // Items are destroyed without a hook; drop rows whose item no longer exists.
    // item_instance.guid holds the low 32 bits of the raw GUID stored here.
    CharacterDatabase.Execute(
        "DELETE ia FROM item_affix ia LEFT JOIN item_instance ii ON (ia.item_guid & 0xFFFFFFFF) = ii.guid "
        "WHERE ii.guid IS NULL");
    CharacterDatabase.Execute(
        "DELETE irs FROM item_reforge_state irs LEFT JOIN item_instance ii ON (irs.item_guid & 0xFFFFFFFF) = ii.guid "
        "WHERE ii.guid IS NULL");
}

bool ItemAffixMgr::Handles(Player const* player) const
{
    return _enabled && player && player->GetSession() && !(_excludeBots && player->GetSession()->IsBot());
}

AffixDefinition const* ItemAffixMgr::GetAffixDef(uint32 id) const
{
    auto it = _defs.find(id);
    return it != _defs.end() ? &it->second : nullptr;
}

AffixDefinition const* ItemAffixMgr::GetAffixDefByKey(std::string const& key) const
{
    auto it = _defsByKey.find(key);
    return it != _defsByKey.end() ? GetAffixDef(it->second) : nullptr;
}

void ItemAffixMgr::LoadSkillAffixes()
{
    if (!sSpellMgr->GetSpellInfo(SKILL_AFFIX_CARRIER_SPELL))
    {
        LOG_ERROR("module", "mod-item-affixes: carrier spell {} is missing from spell_dbc; skill affixes disabled.",
            SKILL_AFFIX_CARRIER_SPELL);
        return;
    }

    std::unordered_map<uint32, std::vector<uint32>> targets;
    if (QueryResult result = WorldDatabase.Query("SELECT affix_id, spell_id FROM affix_skill_target ORDER BY affix_id, spell_id"))
    {
        do
        {
            Field* f = result->Fetch();
            targets[f[0].Get<uint32>()].push_back(f[1].Get<uint32>());
        } while (result->NextRow());
    }

    QueryResult result = WorldDatabase.Query(
        "SELECT id, name, weight, min_quality, affix_key, class_id, spec_id, target_spell, known_spell, "
        "op1, type1, value1, op2, type2, value2, op3, type3, value3, scales_with_crit "
        "FROM affix_template WHERE affix_type = 0 AND weight > 0");
    if (!result)
    {
        LOG_INFO("module", "mod-item-affixes: loaded 0 skill affix(es).");
        return;
    }

    uint32 skipped = 0;
    do
    {
        Field* f = result->Fetch();
        AffixDefinition def;
        def.id             = f[0].Get<uint32>();
        def.name           = f[1].Get<std::string>();
        def.weight         = f[2].Get<uint32>();
        def.minQuality     = f[3].Get<uint8>();
        def.key            = f[4].Get<std::string>();
        def.classId        = f[5].Get<uint8>();
        def.specId         = f[6].Get<uint16>();
        def.targetSpell    = f[7].Get<uint32>();
        def.knownSpell     = f[8].Get<uint32>();
        def.scalesWithCrit = f[18].Get<uint8>() != 0;
        def.affixType      = AFFIX_TYPE_SKILL;

        std::string error;
        for (uint8 i = 0; i < MAX_SKILL_AFFIX_EFFECTS; ++i)
        {
            SkillAffixEffect fx;
            fx.op    = f[9 + i * 3].Get<uint8>();
            uint8 type = f[10 + i * 3].Get<uint8>();
            fx.pct   = type == 1;
            fx.value = f[11 + i * 3].Get<int32>();
            if (!fx.value)
                continue;
            if (fx.op >= MAX_SPELLMOD || type > 1)
                error = Acore::StringFormat("effect {} has op {} type {}", i + 1, fx.op, type);
            def.effects.push_back(fx);
        }

        std::vector<uint32> listed;
        if (auto it = targets.find(def.id); it != targets.end())
            for (uint32 spellId : it->second)
                if (sSpellMgr->GetSpellInfo(spellId))
                    listed.push_back(spellId);
        if (listed.empty() && sSpellMgr->GetSpellInfo(def.targetSpell))
            listed.push_back(def.targetSpell);
        for (uint32 spellId : listed)
            for (uint32 rank = sSpellMgr->GetFirstSpellInChain(spellId); rank; rank = sSpellMgr->GetNextSpellInChain(rank))
                if (std::find(def.targets.begin(), def.targets.end(), rank) == def.targets.end())
                    def.targets.push_back(rank);

        if (def.classId < FIRST_COA_CLASS || def.classId > LAST_COA_CLASS)
            error = Acore::StringFormat("class {} is not a CoA class", def.classId);
        else if (!sSpellMgr->GetSpellInfo(def.targetSpell) || !sSpellMgr->GetSpellInfo(def.knownSpell))
            error = Acore::StringFormat("target spell {} or known spell {} does not exist", def.targetSpell, def.knownSpell);
        else if (def.effects.empty())
            error = "no effect";
        else if (def.name.find("$v") == std::string::npos)
            error = "name has no $v";

        if (!error.empty())
        {
            LOG_ERROR("module", "mod-item-affixes: skill affix {} ({}) skipped: {}.", def.id, def.key, error);
            ++skipped;
            continue;
        }

        _skillPoolByClass[def.classId].push_back(def.id);
        if (!def.key.empty())
            _defsByKey[def.key] = def.id;
        _defs[def.id] = std::move(def);
        ++_skillAffixCount;
    } while (result->NextRow());

    LOG_INFO("module", "mod-item-affixes: loaded {} skill affix(es) for {} class(es), {} skipped; carrier spell {}.",
        _skillAffixCount, _skillPoolByClass.size(), skipped, SKILL_AFFIX_CARRIER_SPELL);
}

// ---------------------------------------------------------------------------
// Storage
// ---------------------------------------------------------------------------

std::vector<AffixSlot> ItemAffixMgr::GetSlots(uint64 itemGuid)
{
    {
        std::lock_guard<std::mutex> guard(_cacheLock);
        auto it = _cache.find(itemGuid);
        if (it != _cache.end())
            return it->second;
    }

    std::vector<AffixSlot> slots;
    if (QueryResult result = CharacterDatabase.Query(
            "SELECT affix_slot, roll_state, affix_id, rolled_value, is_crit FROM item_affix "
            "WHERE item_guid = {} ORDER BY affix_slot", itemGuid))
    {
        do
        {
            Field* f = result->Fetch();
            AffixSlot slot;
            slot.rollState   = f[1].Get<uint8>() == AFFIX_ROLL_APPLIED ? AFFIX_ROLL_APPLIED : AFFIX_ROLL_UNROLLED;
            slot.affixId     = f[2].Get<uint32>();
            slot.rolledValue = f[3].Get<int32>();
            slot.isCrit      = f[4].Get<uint8>() != 0;
            slots.push_back(slot);
        } while (result->NextRow());
    }

    std::lock_guard<std::mutex> guard(_cacheLock);
    return _cache.emplace(itemGuid, std::move(slots)).first->second;
}

// The cache is updated first and is authoritative, so the database writes can be asynchronous;
// one transaction keeps a delete-then-insert in order.
void ItemAffixMgr::StoreSlots(uint64 itemGuid, std::vector<AffixSlot> const& slots, uint8 firstChanged)
{
    {
        std::lock_guard<std::mutex> guard(_cacheLock);
        _cache[itemGuid] = slots;
    }

    CharacterDatabaseTransaction trans = CharacterDatabase.BeginTransaction();
    if (firstChanged == 0)
        trans->Append("DELETE FROM item_affix WHERE item_guid = {}", itemGuid);
    for (uint8 i = firstChanged; i < slots.size(); ++i)
        trans->Append(
            "REPLACE INTO item_affix (item_guid, affix_slot, affix_id, rolled_value, roll_state, is_crit) "
            "VALUES ({}, {}, {}, {}, {}, {})",
            itemGuid, i, slots[i].affixId, slots[i].rolledValue, slots[i].rollState, slots[i].isCrit ? 1 : 0);
    CharacterDatabase.CommitTransaction(trans);
}

// ---------------------------------------------------------------------------
// Rolling
// ---------------------------------------------------------------------------

bool ItemAffixMgr::IsEligibleItem(Item const* item) const
{
    ItemTemplate const* proto = item ? item->GetTemplate() : nullptr;
    if (!proto || proto->ItemLevel == 0 || proto->ScalingStatDistribution)
        return false;
    if (proto->Class != ITEM_CLASS_WEAPON && proto->Class != ITEM_CLASS_ARMOR)
        return false;
    if (proto->Quality < ITEM_QUALITY_UNCOMMON || proto->Quality > ITEM_QUALITY_LEGENDARY)
        return false;

    switch (proto->InventoryType)
    {
        case INVTYPE_NON_EQUIP:
        case INVTYPE_BODY:
        case INVTYPE_TABARD:
        case INVTYPE_BAG:
        case INVTYPE_AMMO:
        case INVTYPE_QUIVER:
        case INVTYPE_THROWN:
            return false;
        default:
            return true;
    }
}

uint8 ItemAffixMgr::GetSlotCount(Item const* item) const
{
    ItemTemplate const* proto = item->GetTemplate();
    uint8 count = proto->Quality >= ITEM_QUALITY_LEGENDARY ? _slotCountLegendary : _slotCount[proto->Quality];
    if (Is2HWeapon(proto))
        count += _twoHanderBonusSlots;
    return std::min(count, MAX_AFFIX_SLOTS);
}

float ItemAffixMgr::GetAffixBudget(Item const* item) const
{
    ItemTemplate const* proto = item->GetTemplate();
    uint32 itemLevel = _maxBudgetItemLevel ? std::min(proto->ItemLevel, _maxBudgetItemLevel) : proto->ItemLevel;
    float fraction = proto->Quality >= ITEM_QUALITY_LEGENDARY ? _budgetFractionLegendary : _budgetFraction[proto->Quality];
    return ComputeItemBudget(itemLevel) * GetSlotBudgetMod(proto->InventoryType) * fraction * _statMultiplier;
}

std::vector<std::pair<uint32, uint32>> ItemAffixMgr::BuildWeightedPool(RollContext const& ctx) const
{
    std::vector<std::pair<uint32, uint32>> pool;
    for (auto const& [id, def] : _defs)
    {
        if (def.IsSkill())
            continue;
        if (def.minQuality > ctx.proto->Quality)
            continue;
        if (!ItemMatchesCategory(ctx.itemCategory, def.itemCategory))
            continue;
        if (def.requiresMana && !ctx.hasMana)
            continue;
        if (IsSummonStat(def.statOp) && (!_summonAffixesEnabled || !ctx.summonSpec || ctx.hasSummonStat))
            continue;
        // No budget floor here: BudgetRange gives every stat at least 1 point, so very low item levels
        // still roll (+1) instead of storing an empty slot.

        uint32 weight = def.weight;
        if (!ctx.profile.anyRole)
        {
            if (def.roleMask && !(def.roleMask & ctx.profile.roleMask))
                continue;

            if (uint8 itemMod = AttributeItemMod(def.statOp))
            {
                // The main attribute rolls at full weight, the spec's other listed attributes (and
                // Spirit for every healer) at the secondary weight, anything else never.
                auto const& stats = ctx.profile.primaryStats;
                auto it = std::find(stats.begin(), stats.end(), itemMod);
                bool const healerSpirit = itemMod == ITEM_MOD_SPIRIT && (ctx.profile.roleMask & AFFIX_ROLE_HEALER);
                if (it == stats.end() && !healerSpirit)
                    continue;
                if (it != stats.begin())
                    weight = weight * _secondaryAttributeWeightPct / 100;
            }
        }

        if (weight)
            pool.emplace_back(id, weight);
    }
    return pool;
}

uint32 ItemAffixMgr::PickAffix(RollContext const& ctx) const
{
    std::vector<std::pair<uint32, uint32>> pool = BuildWeightedPool(ctx);

    // Prefer stats the item does not already carry; allow repeats only when nothing else fits.
    std::vector<std::pair<uint32, uint32>> fresh;
    for (auto const& entry : pool)
        if (!ctx.takenStats.count(_defs.at(entry.first).statOp))
            fresh.push_back(entry);
    std::vector<std::pair<uint32, uint32>> const& candidates = fresh.empty() ? pool : fresh;

    uint32 total = 0;
    for (auto const& [id, weight] : candidates)
        total += weight;
    if (!total)
        return 0;

    uint32 roll = urand(0, total - 1);
    for (auto const& [id, weight] : candidates)
    {
        if (roll < weight)
            return id;
        roll -= weight;
    }
    return 0;
}

bool ItemAffixMgr::PlayerKnowsSkillAffix(Player const* player, AffixDefinition const& def) const
{
    if (!player || !def.IsSkill())
        return false;
    if (player->HasSpellOrTwin(def.knownSpell))
        return true;
    for (uint32 rank = sSpellMgr->GetFirstSpellInChain(def.knownSpell); rank; rank = sSpellMgr->GetNextSpellInChain(rank))
        if (player->HasSpell(rank))
            return true;
    return false;
}

bool ItemAffixMgr::ItemHasSkillAffix(std::vector<AffixSlot> const& slots, size_t ignoreSlot) const
{
    for (size_t i = 0; i < slots.size(); ++i)
        if (i != ignoreSlot && slots[i].rollState == AFFIX_ROLL_APPLIED)
            if (AffixDefinition const* def = GetAffixDef(slots[i].affixId); def && def->IsSkill())
                return true;
    return false;
}

// Blue+ items of CoA classes (12-32), at most one skill affix per item.
bool ItemAffixMgr::RollsSkillAffix(RollContext const& ctx) const
{
    return _skillAffixCount && _skillAffixChance && ctx.proto->Quality >= ITEM_QUALITY_RARE &&
        _skillPoolByClass.count(ctx.player->getClass()) && urand(0, 99) < _skillAffixChance;
}

// Weighted pick among the player's class skill affixes the item quality allows and whose spell
// (any rank) the player knows.
uint32 ItemAffixMgr::PickSkillAffix(RollContext const& ctx) const
{
    auto it = _skillPoolByClass.find(ctx.player->getClass());
    if (it == _skillPoolByClass.end())
        return 0;

    std::vector<std::pair<uint32, uint32>> pool;
    uint32 total = 0;
    for (uint32 id : it->second)
    {
        AffixDefinition const& def = _defs.at(id);
        if (def.minQuality > ctx.proto->Quality || !def.weight || !PlayerKnowsSkillAffix(ctx.player, def))
            continue;
        pool.emplace_back(id, def.weight);
        total += def.weight;
    }
    if (!total)
        return 0;

    uint32 roll = urand(0, total - 1);
    for (auto const& [id, weight] : pool)
    {
        if (roll < weight)
            return id;
        roll -= weight;
    }
    return 0;
}

void ItemAffixMgr::RollValue(RollContext const& ctx, AffixDefinition const& def, int32& value, bool& isCrit) const
{
    if (def.IsSkill())
    {
        isCrit = def.scalesWithCrit && _critRollEnabled && urand(0, 99) < _critRollChance;
        value = isCrit ? SKILL_AFFIX_CRIT_SCALE : SKILL_AFFIX_BASE_SCALE;
        return;
    }

    ValueRange range = BudgetRange(def.statOp, ctx.budget, _budgetMinRoll);
    value = irand(range.minVal, range.maxVal);
    if (Is2HWeapon(ctx.proto))
        value = Boost(value);

    isCrit = _critRollEnabled && urand(0, 99) < _critRollChance;
    if (isCrit)
        value = Boost(value);
}

void ItemAffixMgr::RollItem(Player* player, Item* item)
{
    if (_defs.empty() || !Handles(player) || !IsEligibleItem(item))
        return;

    uint64 itemGuid = item->GetGUID().GetRawValue();
    std::vector<AffixSlot> slots = GetSlots(itemGuid);
    // The stored slot count keeps a bonus slot; it is only rolled when the item gets its first affixes.
    uint8 target = std::max(GetSlotCount(item), uint8(std::min<size_t>(slots.size(), MAX_AFFIX_SLOTS)));
    if (slots.empty() && _bonusSlotChance && urand(0, 99) < _bonusSlotChance)
        target = std::min<uint8>(target + 1, MAX_AFFIX_SLOTS);
    // Slots stored empty (affix_id 0, e.g. by builds that skipped stats worth < 1 point) roll again.
    auto const empty = std::find_if(slots.begin(), slots.end(),
        [](AffixSlot const& slot) { return slot.affixId == 0; });
    slots.erase(empty, slots.end());
    if (slots.size() >= target)
        return;

    RollContext ctx;
    ctx.player       = player;
    ctx.proto        = item->GetTemplate();
    ctx.profile      = CoASpecRoles::Resolve(player);
    ctx.budget       = GetAffixBudget(item);
    ctx.itemCategory = GetItemCategory(ctx.proto);
    ctx.hasMana      = player->GetMaxPower(POWER_MANA) > 0;
    ctx.summonSpec   = IsSummonSpec(ctx.profile.specId);
    for (AffixSlot const& slot : slots)
        if (AffixDefinition const* def = GetAffixDef(slot.affixId); def && !def->IsSkill())
        {
            ctx.takenStats.insert(def->statOp);
            ctx.hasSummonStat |= IsSummonStat(def->statOp);
        }

    uint32 skillId = !ItemHasSkillAffix(slots) && RollsSkillAffix(ctx) ? PickSkillAffix(ctx) : 0;

    auto [luaBag, luaSlot] = GetLuaBagSlot(item);
    uint8 const firstNew = uint8(slots.size());
    while (slots.size() < target)
    {
        AffixSlot slot;
        uint32 id = skillId ? skillId : PickAffix(ctx);
        skillId = 0;
        if (id)
        {
            AffixDefinition const& def = _defs.at(id);
            slot.rollState = AFFIX_ROLL_APPLIED;
            slot.affixId   = id;
            RollValue(ctx, def, slot.rolledValue, slot.isCrit);
            if (!def.IsSkill())
            {
                ctx.takenStats.insert(def.statOp);
                ctx.hasSummonStat |= IsSummonStat(def.statOp);
            }

            // Notification for the optional ItemAffixesToast addon.
            if (slot.isCrit)
                SendAddonMsg(player, Acore::StringFormat("D3ROLL|{}|{}|!{}", luaBag, luaSlot,
                    BuildAffixDisplayString(&def, slot.rolledValue)));
        }
        slots.push_back(slot);
    }

    StoreSlots(itemGuid, slots, firstNew);
    RefreshEquippedItem(player, item);
    SendItemStatus(player, item);
}

void ItemAffixMgr::RollUnrolledEquipped(Player* player)
{
    if (!_rollUnrolledOnEquip || !Handles(player))
        return;
    for (uint8 slot = EQUIPMENT_SLOT_START; slot < EQUIPMENT_SLOT_END; ++slot)
        if (Item* item = player->GetItemByPos(INVENTORY_SLOT_BAG_0, slot))
            RollItem(player, item);
}

void ItemAffixMgr::RerollItem(Player* player, Item* item)
{
    if (!player || !item)
        return;
    uint64 itemGuid = item->GetGUID().GetRawValue();
    StoreSlots(itemGuid, {}, 0);
    CharacterDatabase.Execute("DELETE FROM item_reforge_state WHERE item_guid = {}", itemGuid);
    RefreshEquippedItem(player, item);
    RollItem(player, item);
    SendItemStatus(player, item);
}

bool ItemAffixMgr::GiveSkillAffix(Player* player, Item* item, AffixDefinition const& def, bool crit)
{
    if (!player || !item || !def.IsSkill() || !IsEligibleItem(item))
        return false;
    uint64 itemGuid = item->GetGUID().GetRawValue();
    std::vector<AffixSlot> slots = GetSlots(itemGuid);

    size_t index = slots.size();
    for (size_t i = 0; i < slots.size(); ++i)
        if (AffixDefinition const* other = GetAffixDef(slots[i].affixId); other && other->IsSkill())
            index = i;
    if (index == slots.size() && slots.size() >= MAX_AFFIX_SLOTS)
        index = 0;
    if (index == slots.size())
        slots.emplace_back();

    bool const isCrit = crit && def.scalesWithCrit;
    slots[index] = { AFFIX_ROLL_APPLIED, def.id, isCrit ? SKILL_AFFIX_CRIT_SCALE : SKILL_AFFIX_BASE_SCALE, isCrit };
    StoreSlots(itemGuid, slots, uint8(index));
    RefreshEquippedItem(player, item);
    SendItemStatus(player, item);
    return true;
}

bool ItemAffixMgr::GiveStatAffix(Player* player, Item* item, AffixDefinition const& def)
{
    if (!player || !item || def.IsSkill() || !IsEligibleItem(item))
        return false;
    uint64 itemGuid = item->GetGUID().GetRawValue();
    std::vector<AffixSlot> slots = GetSlots(itemGuid);

    size_t index = slots.size();
    for (size_t i = 0; i < slots.size(); ++i)
    {
        AffixDefinition const* other = GetAffixDef(slots[i].affixId);
        if (!other || other->IsSkill())
            continue;
        if (index == slots.size() || (IsSummonStat(def.statOp) && IsSummonStat(other->statOp)))
            index = i;
    }
    if (index == slots.size() && slots.size() >= MAX_AFFIX_SLOTS)
        index = 0;
    if (index == slots.size())
        slots.emplace_back();

    int32 value = BudgetRange(def.statOp, GetAffixBudget(item), _budgetMinRoll).maxVal;
    if (Is2HWeapon(item->GetTemplate()))
        value = Boost(value);
    slots[index] = { AFFIX_ROLL_APPLIED, def.id, value, false };
    StoreSlots(itemGuid, slots, uint8(index));
    RefreshEquippedItem(player, item);
    SendItemStatus(player, item);
    return true;
}

// ---------------------------------------------------------------------------
// Summon affixes
// ---------------------------------------------------------------------------

void ItemAffixMgr::ApplySummonStat(Player* player, uint8 statOp, int32 value, bool apply)
{
    ItemAffixPlayerData* data = player->CustomData.GetDefault<ItemAffixPlayerData>("ItemAffixData");
    int32 const delta = apply ? value : -value;
    switch (statOp)
    {
        case GSTAT_SUMMON_HEALTH_PCT:       data->summonStats.healthPct      += delta; break;
        case GSTAT_SUMMON_DAMAGE_PCT:       data->summonStats.damagePct      += delta; return;
        case GSTAT_SUMMON_ATTACK_SPEED_PCT: data->summonStats.attackSpeedPct += delta; break;
        default:                            return;
    }
    ScheduleSummonRefresh(player);
}

SummonStatTotals ItemAffixMgr::GetSummonStats(Player const* player) const
{
    if (!_summonAffixesEnabled || !player)
        return {};
    ItemAffixPlayerData const* data = player->CustomData.Get<ItemAffixPlayerData>("ItemAffixData");
    if (!data)
        return {};
    auto cap = [this](int32 value) { return std::clamp(value, 0, _summonCapPct); };
    SummonStatTotals const& raw = data->summonStats;
    return { cap(raw.healthPct), cap(raw.damagePct), cap(raw.attackSpeedPct) };
}

Player* ItemAffixMgr::GetSummonOwner(Unit const* unit)
{
    Creature const* creature = unit ? unit->ToCreature() : nullptr;
    if (!creature)
        return nullptr;
    ObjectGuid const owner = creature->GetOwnerGUID();
    if (!owner.IsPlayer())
        return nullptr;
    Player* player = ObjectAccessor::GetPlayer(*creature, owner);
    return sItemAffixMgr->Handles(player) ? player : nullptr;
}

void ItemAffixMgr::UpdateSummonCarrier(Creature* summon, Player* owner) const
{
    if (!summon || !summon->IsInWorld())
        return;
    SummonStatTotals const stats = GetSummonStats(owner);
    if (!stats.healthPct && !stats.attackSpeedPct)
    {
        summon->RemoveAurasDueToSpell(SUMMON_AFFIX_CARRIER_SPELL);
        return;
    }

    Aura* aura = summon->GetAura(SUMMON_AFFIX_CARRIER_SPELL);
    if (!aura)
        aura = summon->AddAura(SUMMON_AFFIX_CARRIER_SPELL, summon);
    if (!aura)
        return;
    for (auto [index, amount] : { std::pair<uint8, int32>{ EFFECT_0, stats.healthPct },
                                  std::pair<uint8, int32>{ EFFECT_1, stats.attackSpeedPct } })
        if (AuraEffect* effect = aura->GetEffect(index); effect && effect->GetAmount() != amount)
            effect->ChangeAmount(amount);
}

// Guardians and pets get their level stats after entering the world; the carrier follows once that is done.
void ItemAffixMgr::OnSummonAdded(Creature* summon, Player* owner)
{
    if (!_summonAffixesEnabled || !summon || !Handles(owner))
        return;
    ObjectGuid const ownerGuid = owner->GetGUID();
    summon->m_Events.AddEventAtOffset([summon, ownerGuid]()
    {
        if (Player* player = ObjectAccessor::GetPlayer(*summon, ownerGuid))
            sItemAffixMgr->UpdateSummonCarrier(summon, player);
    }, SUMMON_CARRIER_DELAY);
}

void ItemAffixMgr::ScheduleSummonRefresh(Player* player)
{
    if (!_summonAffixesEnabled || !player->IsInWorld())
        return;
    ItemAffixPlayerData* data = player->CustomData.GetDefault<ItemAffixPlayerData>("ItemAffixData");
    if (data->summonRefreshScheduled)
        return;
    data->summonRefreshScheduled = true;
    player->m_Events.AddEventAtOffset([player]()
    {
        if (ItemAffixPlayerData* pending = player->CustomData.Get<ItemAffixPlayerData>("ItemAffixData"))
            pending->summonRefreshScheduled = false;
        if (player->IsInWorld())
            sItemAffixMgr->RefreshSummons(player);
    }, SUMMON_CARRIER_DELAY);
}

void ItemAffixMgr::RefreshSummons(Player* player)
{
    ObjectGuid const guid = player->GetGUID();
    auto ownedBy = [guid](Creature* creature) { return creature->GetOwnerGUID() == guid; };
    std::list<Creature*> summons;
    Acore::CreatureListSearcher<decltype(ownedBy)> searcher(player, summons, ownedBy);
    Cell::VisitObjects(player, searcher, SUMMON_REFRESH_RANGE);
    for (Creature* summon : summons)
        UpdateSummonCarrier(summon, player);
}

// ---------------------------------------------------------------------------
// Applying
// ---------------------------------------------------------------------------

void ItemAffixMgr::ApplyItemAffixes(Player* player, Item* item)
{
    if (!Handles(player) || !item)
        return;
    SetBatch(player, true);
    RemoveItemAffixes(player, item);

    uint64 itemGuid = item->GetGUID().GetRawValue();
    ItemAffixPlayerData* data = player->CustomData.GetDefault<ItemAffixPlayerData>("ItemAffixData");
    for (AffixSlot const& slot : GetSlots(itemGuid))
    {
        if (slot.rollState != AFFIX_ROLL_APPLIED || slot.rolledValue == 0)
            continue;
        AffixDefinition const* def = GetAffixDef(slot.affixId);
        if (!def)
            continue;
        if (def->IsSkill())
        {
            data->activeSkillAffixes[itemGuid].push_back({ def->id, slot.rolledValue });
            data->rebuildPending = true;
            continue;
        }
        ApplyGenericStat(player, def->statOp, slot.rolledValue, true);
        data->activeStatMods[itemGuid].push_back({ def->statOp, slot.rolledValue });
    }
    SetBatch(player, false);
}

void ItemAffixMgr::RemoveItemAffixes(Player* player, Item* item)
{
    if (!player || !item)
        return;
    ItemAffixPlayerData* data = player->CustomData.Get<ItemAffixPlayerData>("ItemAffixData");
    if (!data)
        return;
    uint64 itemGuid = item->GetGUID().GetRawValue();
    if (auto it = data->activeStatMods.find(itemGuid); it != data->activeStatMods.end())
    {
        for (ActiveStatMod const& mod : it->second)
            ApplyGenericStat(player, mod.statOp, mod.value, false);
        data->activeStatMods.erase(it);
    }
    if (data->activeSkillAffixes.erase(itemGuid))
    {
        data->rebuildPending = true;
        if (!data->batchDepth)
            RebuildSkillMods(player);
    }
}

void ItemAffixMgr::SyncAffixes(Player* player)
{
    if (!Handles(player))
        return;

    SetBatch(player, true);
    ItemAffixPlayerData* data = player->CustomData.GetDefault<ItemAffixPlayerData>("ItemAffixData");
    for (auto const& [guid, mods] : data->activeStatMods)
        for (ActiveStatMod const& mod : mods)
            ApplyGenericStat(player, mod.statOp, mod.value, false);
    data->activeStatMods.clear();
    data->activeSkillAffixes.clear();
    data->rebuildPending = true;

    // Same filter as Player::_ApplyAllItemMods.
    for (uint8 slot = EQUIPMENT_SLOT_START; slot < EQUIPMENT_SLOT_END; ++slot)
        if (Item* item = player->GetItemByPos(INVENTORY_SLOT_BAG_0, slot))
            if (!item->IsBroken() && player->CanUseAttackType(Player::GetAttackBySlot(slot)))
                ApplyItemAffixes(player, item);
    SetBatch(player, false);
}

// Nested apply/remove calls rebuild the skill modifiers once, when the outermost call ends.
void ItemAffixMgr::SetBatch(Player* player, bool begin)
{
    ItemAffixPlayerData* data = player->CustomData.GetDefault<ItemAffixPlayerData>("ItemAffixData");
    if (begin)
    {
        ++data->batchDepth;
        return;
    }
    if (data->batchDepth)
        --data->batchDepth;
    if (!data->batchDepth && data->rebuildPending)
        RebuildSkillMods(player);
}

// ---------------------------------------------------------------------------
// Skill affixes: one hidden carrier aura owns every SpellModifier the module adds
// ---------------------------------------------------------------------------

void ItemAffixMgr::ClearSkillMods(Player* player, ItemAffixPlayerData* data)
{
    if (!data)
        return;
    std::vector<SpellModifier*> mods;
    mods.swap(data->skillMods);
    for (SpellModifier* mod : mods)
    {
        if (player)
            player->AddSpellMod(mod, false);
        delete mod;
    }
}

void ItemAffixMgr::RebuildSkillMods(Player* player)
{
    if (!player)
        return;
    BuildSkillMods(player);
    SyncSpellRows(player, false);
}

// Every affix is a server-side targetSpellId modifier (mask 0, so the core sends the client no spell
// modifier) on each of its targets; the client sees it only through the streamed row. Cooldown
// affixes are SPELLMOD_COOLDOWN like any other op.
void ItemAffixMgr::BuildSkillMods(Player* player)
{
    ItemAffixPlayerData* data = player->CustomData.GetDefault<ItemAffixPlayerData>("ItemAffixData");
    data->rebuildPending = false;

    // Modifiers always go before the aura that owns them.
    ClearSkillMods(player, data);
    data->rowEffects.clear();
    std::unordered_map<uint32, std::vector<std::string>> notes;

    // (target spell, op, pct) -> summed value
    std::map<std::tuple<uint32, uint8, bool>, int32> sums;

    bool const usable = Handles(player) && player->IsInWorld() && !player->GetSession()->PlayerLogout();
    if (usable)
    {
        for (auto const& [guid, affixes] : data->activeSkillAffixes)
            for (ActiveSkillAffix const& active : affixes)
            {
                AffixDefinition const* def = GetAffixDef(active.affixId);
                if (!def || !def->IsSkill())
                    continue;
                bool noted = false;
                for (SkillAffixEffect const& fx : def->effects)
                {
                    int32 const value = ScaleSkillValue(fx.value, active.scalePct);
                    for (uint32 spellId : def->targets)
                        sums[{ spellId, fx.op, fx.pct }] += value;
                    if (!noted && !HasRowField(fx.op))
                    {
                        noted = true;
                        std::string const text = BuildAffixDisplayString(def, active.scalePct);
                        for (uint32 spellId : def->targets)
                            notes[spellId].push_back(text);
                    }
                }
            }
    }

    Aura* carrier = player->GetAura(SKILL_AFFIX_CARRIER_SPELL);
    if (sums.empty())
    {
        if (carrier)
        {
            data->removingCarrier = true;
            player->RemoveAura(carrier);
            data->removingCarrier = false;
        }
        return;
    }

    if (!carrier)
        carrier = player->AddAura(SKILL_AFFIX_CARRIER_SPELL, player);
    if (!carrier)
    {
        LOG_ERROR("module", "mod-item-affixes: could not apply carrier aura {} to {}.", SKILL_AFFIX_CARRIER_SPELL,
            player->GetName());
        return;
    }

    for (auto const& [key, sum] : sums)
    {
        auto const& [targetSpellId, op, pct] = key;
        int32 const value = ClampSkillValue(op, pct, sum, sSpellMgr->GetSpellInfo(targetSpellId));
        if (!value)
            continue;
        SpellModifier* mod = new SpellModifier(carrier);
        mod->op            = SpellModOp(op);
        mod->type          = pct ? SPELLMOD_PCT : SPELLMOD_FLAT;
        mod->value         = value;
        mod->mask          = flag96(0, 0, 0);
        mod->spellId       = SKILL_AFFIX_CARRIER_SPELL;
        mod->targetSpellId = targetSpellId;
        data->skillMods.push_back(mod);
        player->AddSpellMod(mod, true);
        if (op < SKILL_ROW_MAX_OPS)
        {
            SkillRowEffects& row = data->rowEffects[targetSpellId];
            (pct ? row.pct : row.flat)[op] += value;
        }
    }
    for (auto& [spellId, texts] : notes)
        data->rowEffects[spellId].notes = std::move(texts);
}

// UnitScript::OnAuraRemove: the aura is unapplied but not deleted yet, so the modifiers that point
// at it are removed now. A removal the module did not ask for (RemoveAllAuras, .unaura) restores
// the carrier shortly after, outside the core's aura loops.
void ItemAffixMgr::OnCarrierRemoved(Player* player)
{
    ItemAffixPlayerData* data = player->CustomData.Get<ItemAffixPlayerData>("ItemAffixData");
    if (!data)
        return;
    ClearSkillMods(player, data);
    if (data->removingCarrier || data->rebuildScheduled || data->activeSkillAffixes.empty() ||
        !player->IsInWorld() || !player->GetSession() || player->GetSession()->PlayerLogout())
        return;

    data->rebuildScheduled = true;
    player->m_Events.AddEventAtOffset([player]()
    {
        if (ItemAffixPlayerData* pending = player->CustomData.Get<ItemAffixPlayerData>("ItemAffixData"))
            pending->rebuildScheduled = false;
        if (player->IsInWorld())
            sItemAffixMgr->RebuildSkillMods(player);
    }, CARRIER_RESTORE_DELAY);
}

// After the affixes of an equipped item changed: swap its stats if the core currently applies the item.
void ItemAffixMgr::RefreshEquippedItem(Player* player, Item* item)
{
    if (!IsEquipped(item))
        return;
    RemoveItemAffixes(player, item);
    if (!item->IsBroken() && player->IsAlive() && player->CanUseAttackType(Player::GetAttackBySlot(item->GetSlot())))
        ApplyItemAffixes(player, item);
}

// ---------------------------------------------------------------------------
// Display and addon messages
// ---------------------------------------------------------------------------

std::string ItemAffixMgr::BuildAffixDisplayString(AffixDefinition const* def, int32 rolledValue) const
{
    if (!def)
        return std::string();
    if (!def->IsSkill())
        return StatText(def->statOp, rolledValue, rolledValue);

    // "<Spell>: <effect>", the number of the first effect scaled by the roll (crit = 150%).
    std::string text = def->name;
    if (size_t pos = text.find("$v"); pos != std::string::npos && !def->effects.empty())
        text.replace(pos, 2, SkillDisplayNumber(def->effects[0], ScaleSkillValue(def->effects[0].value, rolledValue)));
    if (text.size() > MAX_SKILL_TEXT)
        text.resize(MAX_SKILL_TEXT);
    return text;
}

void ItemAffixMgr::SendAddonMsg(Player* player, std::string const& payload)
{
    if (!player || !player->GetSession())
        return;

    // The 3.3.5 client splits "prefix\tbody" and fires CHAT_MSG_ADDON(prefix, body).
    std::string fullMsg = std::string(ADDON_PREFIX) + "\t" + payload;
    WorldPacket data(SMSG_MESSAGECHAT, 1 + 4 + 8 + 4 + 8 + 4 + fullMsg.size() + 2);
    data << uint8(CHAT_MSG_WHISPER);
    data << uint32(LANG_ADDON);
    data << player->GetGUID();
    data << uint32(0);
    data << player->GetGUID();
    data << uint32(fullMsg.size() + 1);
    data << fullMsg;
    data << uint8(0);
    player->GetSession()->SendPacket(&data);
}

// CONFIG|showType|showSpec|showRole|showMainStat|lootMode: every manual-mode selector off, D3 mode.
void ItemAffixMgr::SendConfig(Player* player)
{
    SendAddonMsg(player, "CONFIG|0|0|0|0|1");
}

// State A = applied, I = skill affix whose spell the wearer does not know (shown grey), U = empty.
void ItemAffixMgr::AppendAffixPayload(std::string& msg, std::vector<AffixSlot> const& slots, Player const* wearer) const
{
    for (size_t i = 0; i < slots.size(); ++i)
    {
        AffixSlot const& slot = slots[i];
        std::string text;
        char state = 'U';
        if (slot.rollState == AFFIX_ROLL_APPLIED)
            if (AffixDefinition const* def = GetAffixDef(slot.affixId))
            {
                state = def->IsSkill() && !PlayerKnowsSkillAffix(wearer, *def) ? 'I' : 'A';
                text = (slot.isCrit ? "!" : "") + BuildAffixDisplayString(def, slot.rolledValue);
            }
        msg += Acore::StringFormat("|s{}:{}:{}", i, state, text);
    }
}

// DATA|bag|slot|count|s0:A:text|... -- always sent, even for 0 slots, so the addon stops waiting.
void ItemAffixMgr::SendItemStatus(Player* player, Item* item)
{
    if (!player || !item)
        return;
    std::vector<AffixSlot> slots = GetSlots(item->GetGUID().GetRawValue());
    auto [luaBag, luaSlot] = GetLuaBagSlot(item);
    std::string msg = Acore::StringFormat("DATA|{}|{}|{}", luaBag, luaSlot, slots.size());
    AppendAffixPayload(msg, slots, player);
    SendAddonMsg(player, msg);
}

// OnPlayerLearnSpell/OnPlayerForgotSpell (spec switches learn and forget many at once): the I/A state
// of carried skill affix items is resent once, a moment later.
void ItemAffixMgr::ScheduleSkillStatusRefresh(Player* player)
{
    if (!_skillAffixCount || !Handles(player) || !player->IsInWorld())
        return;
    ItemAffixPlayerData* data = player->CustomData.GetDefault<ItemAffixPlayerData>("ItemAffixData");
    if (data->statusScheduled)
        return;
    data->statusScheduled = true;
    player->m_Events.AddEventAtOffset([player]()
    {
        if (ItemAffixPlayerData* pending = player->CustomData.Get<ItemAffixPlayerData>("ItemAffixData"))
            pending->statusScheduled = false;
        if (!player->IsInWorld())
            return;
        ForEachCarriedItem(player, [&](Item* item)
        {
            if (sItemAffixMgr->ItemHasSkillAffix(sItemAffixMgr->GetSlots(item->GetGUID().GetRawValue())))
                sItemAffixMgr->SendItemStatus(player, item);
        });
    }, STATUS_REFRESH_DELAY);
}

void ItemAffixMgr::RefreshAllItemStatus(Player* player)
{
    if (!player)
        return;
    ForEachCarriedItem(player, [&](Item* item)
    {
        if (!GetSlots(item->GetGUID().GetRawValue()).empty())
            SendItemStatus(player, item);
    });
}

void ItemAffixMgr::HandleAddonMessage(Player* player, std::string const& payload)
{
    if (!player || payload.empty())
        return;

    std::vector<std::string_view> parts = Acore::Tokenize(payload, '|', false);
    if (parts.empty())
        return;
    std::string const cmd(parts[0]);

    if (cmd == "CONFIG")
    {
        SendConfig(player);
        return;
    }

    if (cmd == "ALLDATA")
    {
        RefreshAllItemStatus(player);
        return;
    }

    // PEEK|uniqueId -- any item by the GUID counter in its link (used for links in chat).
    if (cmd == "PEEK")
    {
        Optional<uint32> uid = parts.size() >= 2 ? Acore::StringTo<uint32>(parts[1]) : Optional<uint32>();
        if (!uid)
            return;
        std::vector<AffixSlot> slots = GetSlots(ObjectGuid(HighGuid::Item, *uid).GetRawValue());
        std::string msg = Acore::StringFormat("PEEKDATA|{}|{}", *uid, slots.size());
        AppendAffixPayload(msg, slots, player);
        SendAddonMsg(player, msg);
        return;
    }

    // PEEKUNIT|name|luaSlot and PEEKUNITALL|name -- inspect. Limited to nearby players in the same map.
    if (cmd == "PEEKUNIT" || cmd == "PEEKUNITALL")
    {
        if (parts.size() < 2)
            return;
        std::string targetName(parts[1]);
        Player* target = ObjectAccessor::FindPlayerByName(targetName);
        bool const nearby = target && target->IsInWorld() && target->IsInMap(player) &&
            target->IsWithinDistInMap(player, PEEK_UNIT_MAX_DISTANCE);

        auto sendSlot = [&](uint8 luaSlot)
        {
            Item* targetItem = nearby ? target->GetItemByPos(INVENTORY_SLOT_BAG_0, uint8(luaSlot - 1)) : nullptr;
            std::vector<AffixSlot> slots = targetItem ? GetSlots(targetItem->GetGUID().GetRawValue()) : std::vector<AffixSlot>();
            std::string msg = Acore::StringFormat("INSPECTDATA|{}|{}|{}", targetName, luaSlot, slots.size());
            AppendAffixPayload(msg, slots, nearby ? target : player);
            SendAddonMsg(player, msg);
        };

        if (cmd == "PEEKUNIT")
        {
            Optional<uint8> luaSlot = parts.size() >= 3 ? Acore::StringTo<uint8>(parts[2]) : Optional<uint8>();
            if (luaSlot && *luaSlot >= 1 && *luaSlot <= EQUIPMENT_SLOT_END)
                sendSlot(*luaSlot);
        }
        else if (nearby)
        {
            for (uint8 cppSlot = EQUIPMENT_SLOT_START; cppSlot < EQUIPMENT_SLOT_END; ++cppSlot)
                if (target->GetItemByPos(INVENTORY_SLOT_BAG_0, cppSlot))
                    sendSlot(uint8(cppSlot + 1));
        }
        return;
    }

    // PEEKAUCTION|owner|itemId|buyout|auctionType|index|groupOffset -> AUCTIONDATA|owner|itemId|buyout|type|index|count|...
    if (cmd == "PEEKAUCTION")
    {
        if (parts.size() < 3)
            return;
        std::string ownerName(parts[1]);
        Optional<uint32> itemId = Acore::StringTo<uint32>(parts[2]);
        if (!itemId)
            return;
        uint32 buyout = parts.size() >= 4 ? Acore::StringTo<uint32>(parts[3]).value_or(0) : 0;
        std::string auctionType = parts.size() >= 5 ? std::string(parts[4]) : "list";
        uint32 auctionIndex = parts.size() >= 6 ? Acore::StringTo<uint32>(parts[5]).value_or(0) : 0;
        uint32 groupOffset  = parts.size() >= 7 ? Acore::StringTo<uint32>(parts[6]).value_or(0) : 0;
        if (auctionType != "list" && auctionType != "owner" && auctionType != "bidder")
            auctionType = "list";

        std::vector<AffixSlot> slots;
        // The name comes from the client: only a valid character name reaches the query, escaped.
        std::string safeName = ownerName;
        if (normalizePlayerName(safeName))
        {
            CharacterDatabase.EscapeString(safeName);
            QueryResult result = buyout
                ? CharacterDatabase.Query(
                    "SELECT ah.itemguid FROM auctionhouse ah JOIN item_instance ii ON ii.guid = ah.itemguid "
                    "JOIN characters c ON c.guid = ah.itemowner "
                    "WHERE c.name = '{}' AND ii.itemEntry = {} AND ah.buyoutprice = {} ORDER BY ah.id LIMIT 1 OFFSET {}",
                    safeName, *itemId, buyout, groupOffset)
                : CharacterDatabase.Query(
                    "SELECT ah.itemguid FROM auctionhouse ah JOIN item_instance ii ON ii.guid = ah.itemguid "
                    "JOIN characters c ON c.guid = ah.itemowner "
                    "WHERE c.name = '{}' AND ii.itemEntry = {} ORDER BY ah.id LIMIT 1 OFFSET {}",
                    safeName, *itemId, groupOffset);
            if (result)
                slots = GetSlots(ObjectGuid(HighGuid::Item, result->Fetch()[0].Get<uint32>()).GetRawValue());
        }

        std::string msg = Acore::StringFormat("AUCTIONDATA|{}|{}|{}|{}|{}|{}",
            ownerName, *itemId, buyout, auctionType, auctionIndex, slots.size());
        AppendAffixPayload(msg, slots, player);
        SendAddonMsg(player, msg);
        return;
    }

    // TRADEPEEK|luaSlot (partner) / MYTRADEPEEK|luaSlot (own) -> (MY)TRADEGUID|luaSlot|counter|count|...
    if (cmd == "TRADEPEEK" || cmd == "MYTRADEPEEK")
    {
        Optional<uint8> luaSlot = parts.size() >= 2 ? Acore::StringTo<uint8>(parts[1]) : Optional<uint8>();
        if (!luaSlot || *luaSlot < 1 || *luaSlot > TRADE_SLOT_TRADED_COUNT)
            return;
        char const* reply = cmd == "TRADEPEEK" ? "TRADEGUID" : "MYTRADEGUID";

        TradeData* trade = player->GetTradeData();
        if (trade && cmd == "TRADEPEEK")
            trade = trade->GetTraderData();
        Item* item = trade ? trade->GetItem(TradeSlots(*luaSlot - 1)) : nullptr;
        if (!item)
        {
            SendAddonMsg(player, Acore::StringFormat("{}|{}|0|0", reply, *luaSlot));
            return;
        }
        uint64 rawGuid = item->GetGUID().GetRawValue();
        std::vector<AffixSlot> slots = GetSlots(rawGuid);
        std::string msg = Acore::StringFormat("{}|{}|{}|{}", reply, *luaSlot, uint32(rawGuid), slots.size());
        AppendAffixPayload(msg, slots, player);
        SendAddonMsg(player, msg);
        return;
    }

    // Commands addressing one of the player's own items: CMD|luaBag|luaSlot|...
    if (parts.size() < 3)
        return;
    Optional<uint8> luaBag = Acore::StringTo<uint8>(parts[1]);
    Optional<uint8> luaSlot = Acore::StringTo<uint8>(parts[2]);
    if (!luaBag || !luaSlot)
        return;
    Item* item = GetItemByLuaBagSlot(player, *luaBag, *luaSlot);
    if (!item)
        return;
    auto [bag, slot] = GetLuaBagSlot(item);
    auto sendError = [&](char const* reason)
    {
        SendAddonMsg(player, Acore::StringFormat("ERR|{}|{}|{}", bag, slot, reason));
    };

    if (cmd == "DATA")
    {
        SendItemStatus(player, item);
        return;
    }

    if (cmd == "ROLL" || cmd == "PICK" || cmd == "REROLL" || cmd == "LOCK")
    {
        sendError("Affixes are rolled automatically on this realm.");
        return;
    }

    if (cmd == "REFORGE_STATUS")
    {
        std::vector<AffixSlot> slots = GetSlots(item->GetGUID().GetRawValue());
        ReforgeState state = GetReforgeState(item->GetGUID().GetRawValue());
        uint32 cost = GetReforgeCost(item->GetTemplate(), state.exists ? state.rerollCount : 0);
        std::string msg = Acore::StringFormat("REFORGESTATUS|{}|{}|{}|{}|{}",
            bag, slot, slots.size(), state.exists ? uint32(state.lockedSlot) : 255u, cost);
        for (size_t i = 0; i < slots.size(); ++i)
        {
            AffixDefinition const* def = slots[i].rollState == AFFIX_ROLL_APPLIED ? GetAffixDef(slots[i].affixId) : nullptr;
            msg += Acore::StringFormat("|s{}:{}:{}", i, def ? 'A' : 'X',
                def ? BuildAffixDisplayString(def, slots[i].rolledValue) : std::string());
        }
        SendAddonMsg(player, msg);
        return;
    }

    if (cmd == "REFORGE_ROLL")
    {
        Optional<uint8> affixSlot = parts.size() >= 4 ? Acore::StringTo<uint8>(parts[3]) : Optional<uint8>();
        if (!affixSlot)
            return;
        std::vector<ReforgeOption> opts;
        switch (RollReforgeOptions(player, item, *affixSlot, &opts))
        {
            case ReforgeRollResult::OK:
                break;
            case ReforgeRollResult::ERR_WRONG_SLOT:
                sendError("This item is already locked to a different line.");
                return;
            case ReforgeRollResult::ERR_INSUFFICIENT_GOLD:
                sendError("You don't have enough gold.");
                return;
            default:
                sendError("That line hasn't been rolled yet.");
                return;
        }
        std::string msg = Acore::StringFormat("REFORGEOPTS|{}|{}|{}", bag, slot, *affixSlot);
        for (ReforgeOption const& opt : opts)
            msg += "|" + std::string(opt.isCrit ? "!" : "") + BuildAffixDisplayString(GetAffixDef(opt.affixId), opt.rolledValue);
        SendAddonMsg(player, msg);
        return;
    }

    if (cmd == "REFORGE_PICK")
    {
        Optional<uint8> affixSlot = parts.size() >= 5 ? Acore::StringTo<uint8>(parts[3]) : Optional<uint8>();
        Optional<uint32> optIdx = parts.size() >= 5 ? Acore::StringTo<uint32>(parts[4]) : Optional<uint32>();
        if (!affixSlot || !optIdx)
            return;
        switch (CommitReforgePick(player, item, *affixSlot, *optIdx))
        {
            case ReforgePickResult::OK:                break;
            case ReforgePickResult::ERR_WRONG_SLOT:    sendError("This item is locked to a different line."); break;
            case ReforgePickResult::ERR_INVALID_INDEX: sendError("That option is no longer valid."); break;
            default:                                   sendError("Nothing to pick -- roll first."); break;
        }
        return;
    }

    // REFORGE_PREVIEW|bag|slot|affixSlot: every value range the slot could reforge into, chunked.
    if (cmd == "REFORGE_PREVIEW")
    {
        Optional<uint8> affixSlot = parts.size() >= 4 ? Acore::StringTo<uint8>(parts[3]) : Optional<uint8>();
        if (!affixSlot)
            return;
        std::vector<AffixSlot> slots = GetSlots(item->GetGUID().GetRawValue());
        if (*affixSlot >= slots.size() || slots[*affixSlot].rollState != AFFIX_ROLL_APPLIED)
        {
            sendError("That line hasn't been rolled yet.");
            return;
        }

        RollContext ctx;
        ctx.player       = player;
        ctx.proto        = item->GetTemplate();
        ctx.profile      = CoASpecRoles::Resolve(player);
        ctx.budget       = GetAffixBudget(item);
        ctx.itemCategory = GetItemCategory(ctx.proto);
        ctx.hasMana      = player->GetMaxPower(POWER_MANA) > 0;
        ctx.summonSpec   = IsSummonSpec(ctx.profile.specId);
        for (size_t i = 0; i < slots.size(); ++i)
            if (AffixDefinition const* def = GetAffixDef(slots[i].affixId); i != *affixSlot && def && !def->IsSkill())
                ctx.hasSummonStat |= IsSummonStat(def->statOp);

        std::vector<std::string> texts;
        AffixDefinition const* currentDef = GetAffixDef(slots[*affixSlot].affixId);
        bool const skillSlot = currentDef && currentDef->IsSkill();
        if (skillSlot || (!ItemHasSkillAffix(slots, *affixSlot) && _skillAffixChance && ctx.proto->Quality >= ITEM_QUALITY_RARE))
            if (auto pool = _skillPoolByClass.find(player->getClass()); pool != _skillPoolByClass.end())
                for (uint32 id : pool->second)
                {
                    AffixDefinition const& def = _defs.at(id);
                    if (def.minQuality <= ctx.proto->Quality && PlayerKnowsSkillAffix(player, def))
                        texts.push_back(BuildAffixDisplayString(&def, SKILL_AFFIX_BASE_SCALE));
                }
        for (auto const& [id, weight] : skillSlot ? std::vector<std::pair<uint32, uint32>>() : BuildWeightedPool(ctx))
        {
            AffixDefinition const& def = _defs.at(id);
            ValueRange range = BudgetRange(def.statOp, ctx.budget, _budgetMinRoll);
            if (Is2HWeapon(ctx.proto))
                range = { Boost(range.minVal), Boost(range.maxVal) };
            texts.push_back(StatText(def.statOp, range.minVal, range.maxVal));
        }
        std::sort(texts.begin(), texts.end());

        // Addon messages are capped at 255 bytes: header plus chunks the addon reassembles.
        constexpr size_t ChunkBudget = 180;
        std::vector<std::string> chunks;
        std::string current;
        for (std::string const& text : texts)
        {
            if (!current.empty() && current.size() + 1 + text.size() > ChunkBudget)
            {
                chunks.push_back(current);
                current.clear();
            }
            current += (current.empty() ? "" : "|") + text;
        }
        if (!current.empty())
            chunks.push_back(current);

        SendAddonMsg(player, Acore::StringFormat("REFORGEPREVIEW|{}|{}|{}|{}", bag, slot, *affixSlot, chunks.size()));
        for (size_t i = 0; i < chunks.size(); ++i)
            SendAddonMsg(player, Acore::StringFormat("REFORGEPREVIEWDATA|{}|{}|{}|{}|{}|{}",
                bag, slot, *affixSlot, i, chunks.size(), chunks[i]));
    }
}

// ---------------------------------------------------------------------------
// Reforge: pay to reroll one applied line; the item locks to that line forever.
// ---------------------------------------------------------------------------

ReforgeState ItemAffixMgr::GetReforgeState(uint64 itemGuid) const
{
    ReforgeState state;
    if (QueryResult result = CharacterDatabase.Query(
            "SELECT locked_slot, reroll_count FROM item_reforge_state WHERE item_guid = {}", itemGuid))
    {
        state.exists      = true;
        state.lockedSlot  = result->Fetch()[0].Get<uint8>();
        state.rerollCount = result->Fetch()[1].Get<uint32>();
    }
    return state;
}

uint32 ItemAffixMgr::GetReforgeCost(ItemTemplate const* proto, uint32 timesAlreadyReforged) const
{
    uint32 const quality = proto->Quality;
    double cost = quality >= ITEM_QUALITY_LEGENDARY ? _reforgeBaseCostLegendary
        : _reforgeBaseCost[std::min<uint32>(quality, ITEM_QUALITY_EPIC)];
    if (_reforgeReferenceItemLevel)
    {
        uint32 itemLevel = _maxBudgetItemLevel ? std::min(proto->ItemLevel, _maxBudgetItemLevel) : proto->ItemLevel;
        cost = cost * std::max<uint32>(itemLevel, 1) / _reforgeReferenceItemLevel;
    }
    cost *= 1.0 + timesAlreadyReforged * _reforgeCostIncreasePct / 100.0;
    return std::max<uint32>(1, uint32(std::lround(cost)));
}

ReforgeRollResult ItemAffixMgr::RollReforgeOptions(Player* player, Item* item, uint8 affixSlot,
                                                   std::vector<ReforgeOption>* outOptions)
{
    if (!player || !item || !IsEligibleItem(item))
        return ReforgeRollResult::ERR_NOT_APPLIED;

    uint64 itemGuid = item->GetGUID().GetRawValue();
    std::vector<AffixSlot> slots = GetSlots(itemGuid);
    if (affixSlot >= slots.size() || slots[affixSlot].rollState != AFFIX_ROLL_APPLIED || !GetAffixDef(slots[affixSlot].affixId))
        return ReforgeRollResult::ERR_NOT_APPLIED;

    ReforgeState state = GetReforgeState(itemGuid);
    if (state.exists && state.lockedSlot != affixSlot)
        return ReforgeRollResult::ERR_WRONG_SLOT;

    ItemTemplate const* proto = item->GetTemplate();
    uint32 cost = GetReforgeCost(proto, state.exists ? state.rerollCount : 0);
    if (!player->HasEnoughMoney(cost))
        return ReforgeRollResult::ERR_INSUFFICIENT_GOLD;
    player->ModifyMoney(-int32(cost));

    RollContext ctx;
    ctx.player       = player;
    ctx.proto        = proto;
    ctx.profile      = CoASpecRoles::Resolve(player);
    ctx.budget       = GetAffixBudget(item);
    ctx.itemCategory = GetItemCategory(proto);
    ctx.hasMana      = player->GetMaxPower(POWER_MANA) > 0;
    ctx.summonSpec   = IsSummonSpec(ctx.profile.specId);
    for (size_t i = 0; i < slots.size(); ++i)
        if (i != affixSlot)
            if (AffixDefinition const* def = GetAffixDef(slots[i].affixId); def && !def->IsSkill())
            {
                ctx.takenStats.insert(def->statOp);
                ctx.hasSummonStat |= IsSummonStat(def->statOp);
            }

    // A skill line reforges into skill affixes only; a stat line can turn into one (same chance and
    // pool as rolling) while no other line of the item is a skill affix.
    bool const skillSlot = GetAffixDef(slots[affixSlot].affixId)->IsSkill();
    bool const skillAllowed = !ItemHasSkillAffix(slots, affixSlot);

    // The current value always comes first, so closing the window keeps the item as it was.
    std::vector<ReforgeOption> opts{ { slots[affixSlot].affixId, slots[affixSlot].rolledValue, slots[affixSlot].isCrit } };
    uint8 count = proto->Quality >= ITEM_QUALITY_LEGENDARY ? _reforgeOptionsLegendary
        : _reforgeOptions[std::min<uint32>(proto->Quality, ITEM_QUALITY_EPIC)];
    for (uint8 i = 0; i < count; ++i)
    {
        uint32 id = skillSlot || (skillAllowed && RollsSkillAffix(ctx)) ? PickSkillAffix(ctx) : 0;
        if (!id && !skillSlot)
            id = PickAffix(ctx);
        if (!id)
            continue;
        ReforgeOption opt{ id, 0, false };
        RollValue(ctx, _defs.at(id), opt.rolledValue, opt.isCrit);
        opts.push_back(opt);
    }

    uint32 rerollCount = state.exists ? state.rerollCount + 1 : 1;
    CharacterDatabase.DirectExecute(
        "INSERT INTO item_reforge_state (item_guid, locked_slot, reroll_count, pending_opts) VALUES ({}, {}, {}, '{}') "
        "ON DUPLICATE KEY UPDATE reroll_count = VALUES(reroll_count), pending_opts = VALUES(pending_opts)",
        itemGuid, affixSlot, rerollCount, SerializeOptions(opts));

    if (outOptions)
        *outOptions = std::move(opts);
    return ReforgeRollResult::OK;
}

ReforgePickResult ItemAffixMgr::CommitReforgePick(Player* player, Item* item, uint8 affixSlot, uint32 optIdx)
{
    if (!player || !item)
        return ReforgePickResult::ERR_NO_PENDING;

    uint64 itemGuid = item->GetGUID().GetRawValue();
    QueryResult result = CharacterDatabase.Query(
        "SELECT locked_slot, pending_opts FROM item_reforge_state WHERE item_guid = {}", itemGuid);
    if (!result)
        return ReforgePickResult::ERR_NO_PENDING;
    if (result->Fetch()[0].Get<uint8>() != affixSlot)
        return ReforgePickResult::ERR_WRONG_SLOT;

    std::vector<ReforgeOption> opts = ParseOptions(result->Fetch()[1].Get<std::string>());
    if (opts.empty())
        return ReforgePickResult::ERR_NO_PENDING;
    if (optIdx >= opts.size() || !GetAffixDef(opts[optIdx].affixId))
        return ReforgePickResult::ERR_INVALID_INDEX;

    std::vector<AffixSlot> slots = GetSlots(itemGuid);
    if (affixSlot >= slots.size())
        return ReforgePickResult::ERR_INVALID_INDEX;
    slots[affixSlot] = { AFFIX_ROLL_APPLIED, opts[optIdx].affixId, opts[optIdx].rolledValue, opts[optIdx].isCrit };
    StoreSlots(itemGuid, slots, affixSlot);
    CharacterDatabase.Execute("UPDATE item_reforge_state SET pending_opts = '' WHERE item_guid = {}", itemGuid);

    RefreshEquippedItem(player, item);
    SendItemStatus(player, item);
    return ReforgePickResult::OK;
}
