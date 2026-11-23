#include "ItemAffix.h"

#include "Bag.h"
#include "Config.h"
#include "DatabaseEnv.h"
#include "Item.h"
#include "ItemTemplate.h"
#include "Log.h"
#include "ObjectAccessor.h"
#include "ObjectGuid.h"
#include "ObjectMgr.h"
#include "Opcodes.h"
#include "Player.h"
#include "Random.h"
#include "StringConvert.h"
#include "StringFormat.h"
#include "Tokenize.h"
#include "TradeData.h"
#include "WorldPacket.h"
#include "WorldSession.h"
#include <algorithm>
#include <cmath>
#include <unordered_set>

namespace
{
constexpr char const* ADDON_PREFIX = "AFXM";
constexpr uint8 MAX_AFFIX_SLOTS = 6;
constexpr float PEEK_UNIT_MAX_DISTANCE = 100.0f;

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
    return statOp < GSTAT_MAX ? StatNames[statOp] : "Unknown";
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

struct ItemAffixMgr::RollContext
{
    Player*                 player = nullptr;
    ItemTemplate const*     proto = nullptr;
    AffixSpecProfile        profile;
    float                   budget = 0.0f;
    uint8                   itemCategory = ITEM_CAT_ANY;
    bool                    hasMana = false;
    std::unordered_set<uint8> takenStats;  // stats already on the item; avoided while alternatives exist
};

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
}

void ItemAffixMgr::LoadAffixTemplates()
{
    _defs.clear();
    {
        std::lock_guard<std::mutex> guard(_cacheLock);
        _cache.clear();
    }

    if (!_enabled)
    {
        LOG_INFO("module", "mod-item-affixes: disabled (ItemAffixes.Enable = 0).");
        return;
    }

    QueryResult result = WorldDatabase.Query(
        "SELECT id, name, weight, stat_op, role_mask, item_category, min_quality, requires_mana "
        "FROM affix_template WHERE weight > 0");
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
            if (def.statOp >= GSTAT_MAX)
            {
                LOG_ERROR("module", "mod-item-affixes: affix {} has unsupported stat_op {}, skipped.", def.id, def.statOp);
                continue;
            }
            _defs[def.id] = std::move(def);
        } while (result->NextRow());
    }
    LOG_INFO("module", "mod-item-affixes: loaded {} affix template(s).", _defs.size());

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
        if (def.minQuality > ctx.proto->Quality)
            continue;
        if (!ItemMatchesCategory(ctx.itemCategory, def.itemCategory))
            continue;
        if (def.requiresMana && !ctx.hasMana)
            continue;
        if (int32(ctx.budget / GetStatCost(def.statOp)) < 1)
            continue;

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

void ItemAffixMgr::RollValue(RollContext const& ctx, AffixDefinition const& def, int32& value, bool& isCrit) const
{
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
    uint8 const target = GetSlotCount(item);
    if (slots.size() >= target)
        return;

    RollContext ctx;
    ctx.player       = player;
    ctx.proto        = item->GetTemplate();
    ctx.profile      = CoASpecRoles::Resolve(player);
    ctx.budget       = GetAffixBudget(item);
    ctx.itemCategory = GetItemCategory(ctx.proto);
    ctx.hasMana      = player->GetMaxPower(POWER_MANA) > 0;
    for (AffixSlot const& slot : slots)
        if (AffixDefinition const* def = GetAffixDef(slot.affixId))
            ctx.takenStats.insert(def->statOp);

    auto [luaBag, luaSlot] = GetLuaBagSlot(item);
    uint8 const firstNew = uint8(slots.size());
    while (slots.size() < target)
    {
        AffixSlot slot;
        if (uint32 id = PickAffix(ctx))
        {
            AffixDefinition const& def = _defs.at(id);
            slot.rollState = AFFIX_ROLL_APPLIED;
            slot.affixId   = id;
            RollValue(ctx, def, slot.rolledValue, slot.isCrit);
            ctx.takenStats.insert(def.statOp);

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

// ---------------------------------------------------------------------------
// Applying
// ---------------------------------------------------------------------------

void ItemAffixMgr::ApplyItemAffixes(Player* player, Item* item)
{
    if (!Handles(player) || !item)
        return;
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
        ApplyGenericStat(player, def->statOp, slot.rolledValue, true);
        data->activeStatMods[itemGuid].push_back({ def->statOp, slot.rolledValue });
    }
}

void ItemAffixMgr::RemoveItemAffixes(Player* player, Item* item)
{
    if (!player || !item)
        return;
    ItemAffixPlayerData* data = player->CustomData.Get<ItemAffixPlayerData>("ItemAffixData");
    if (!data)
        return;
    auto it = data->activeStatMods.find(item->GetGUID().GetRawValue());
    if (it == data->activeStatMods.end())
        return;
    for (ActiveStatMod const& mod : it->second)
        ApplyGenericStat(player, mod.statOp, mod.value, false);
    data->activeStatMods.erase(it);
}

void ItemAffixMgr::SyncAffixes(Player* player)
{
    if (!Handles(player))
        return;

    if (ItemAffixPlayerData* data = player->CustomData.Get<ItemAffixPlayerData>("ItemAffixData"))
    {
        for (auto const& [guid, mods] : data->activeStatMods)
            for (ActiveStatMod const& mod : mods)
                ApplyGenericStat(player, mod.statOp, mod.value, false);
        data->activeStatMods.clear();
    }

    // Same filter as Player::_ApplyAllItemMods.
    for (uint8 slot = EQUIPMENT_SLOT_START; slot < EQUIPMENT_SLOT_END; ++slot)
        if (Item* item = player->GetItemByPos(INVENTORY_SLOT_BAG_0, slot))
            if (!item->IsBroken() && player->CanUseAttackType(Player::GetAttackBySlot(slot)))
                ApplyItemAffixes(player, item);
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
    return def ? Acore::StringFormat("+{} {}", rolledValue, GetStatName(def->statOp)) : std::string();
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

void ItemAffixMgr::AppendAffixPayload(std::string& msg, std::vector<AffixSlot> const& slots) const
{
    for (size_t i = 0; i < slots.size(); ++i)
    {
        AffixSlot const& slot = slots[i];
        std::string text;
        char state = 'U';
        if (slot.rollState == AFFIX_ROLL_APPLIED)
            if (AffixDefinition const* def = GetAffixDef(slot.affixId))
            {
                state = 'A';
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
    AppendAffixPayload(msg, slots);
    SendAddonMsg(player, msg);
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
        AppendAffixPayload(msg, slots);
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
            AppendAffixPayload(msg, slots);
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
        AppendAffixPayload(msg, slots);
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
        AppendAffixPayload(msg, slots);
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
        uint32 cost = GetReforgeCost(item->GetTemplate()->Quality, state.exists ? state.rerollCount : 0);
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

        std::vector<std::string> texts;
        for (auto const& [id, weight] : BuildWeightedPool(ctx))
        {
            AffixDefinition const& def = _defs.at(id);
            ValueRange range = BudgetRange(def.statOp, ctx.budget, _budgetMinRoll);
            if (Is2HWeapon(ctx.proto))
                range = { Boost(range.minVal), Boost(range.maxVal) };
            texts.push_back(range.minVal == range.maxVal
                ? Acore::StringFormat("+{} {}", range.minVal, GetStatName(def.statOp))
                : Acore::StringFormat("+{}-{} {}", range.minVal, range.maxVal, GetStatName(def.statOp)));
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

uint32 ItemAffixMgr::GetReforgeCost(uint32 itemQuality, uint32 timesAlreadyReforged) const
{
    uint32 base = itemQuality >= ITEM_QUALITY_LEGENDARY ? _reforgeBaseCostLegendary
        : _reforgeBaseCost[std::min<uint32>(itemQuality, ITEM_QUALITY_EPIC)];
    return uint32(base * (1.0 + timesAlreadyReforged * 0.5));
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
    uint32 cost = GetReforgeCost(proto->Quality, state.exists ? state.rerollCount : 0);
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
    for (size_t i = 0; i < slots.size(); ++i)
        if (i != affixSlot)
            if (AffixDefinition const* def = GetAffixDef(slots[i].affixId))
                ctx.takenStats.insert(def->statOp);

    // The current value always comes first, so closing the window keeps the item as it was.
    std::vector<ReforgeOption> opts{ { slots[affixSlot].affixId, slots[affixSlot].rolledValue, slots[affixSlot].isCrit } };
    uint8 count = proto->Quality >= ITEM_QUALITY_LEGENDARY ? _reforgeOptionsLegendary
        : _reforgeOptions[std::min<uint32>(proto->Quality, ITEM_QUALITY_EPIC)];
    for (uint8 i = 0; i < count; ++i)
        if (uint32 id = PickAffix(ctx))
        {
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
