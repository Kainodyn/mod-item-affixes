#ifndef MOD_ITEM_AFFIXES_ITEM_AFFIX_H
#define MOD_ITEM_AFFIXES_ITEM_AFFIX_H

#include "CoASpecRoles.h"
#include "DataMap.h"
#include "ObjectGuid.h"
#include "Define.h"
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

class Item;
class Player;

// Item category filter for affix_template.item_category.
enum AffixItemCategory : uint8
{
    ITEM_CAT_ANY       = 0,
    ITEM_CAT_WEAPON_1H = 1,  // one-handed weapons and shields
    ITEM_CAT_WEAPON_2H = 2,  // two-handed and ranged weapons
    ITEM_CAT_WEAPON    = 3,  // any weapon
    ITEM_CAT_ARMOR     = 4,  // armor slots, boots included
    ITEM_CAT_JEWELRY   = 5,  // neck, finger, trinket
    ITEM_CAT_WAND      = 6,  // INVTYPE_RANGEDRIGHT
    ITEM_CAT_BOOTS     = 7,  // boots only
};

// affix_template.stat_op. Values match upstream so existing rows keep their meaning; gaps are
// upstream stats removed for CoA (Move Speed 18, Life Leech 19, Hp5 20, Damage Reduction 21,
// pet stats 22-26, Max Health 27) and Armor Penetration (17).
enum GenericStatOp : uint8
{
    GSTAT_STAMINA          = 0,
    GSTAT_STRENGTH         = 1,
    GSTAT_AGILITY          = 2,
    GSTAT_INTELLECT        = 3,
    GSTAT_SPIRIT           = 4,
    GSTAT_ATTACK_POWER     = 5,
    GSTAT_RANGED_AP        = 6,
    GSTAT_SPELL_POWER      = 7,
    GSTAT_MP5              = 8,
    GSTAT_ARMOR            = 9,
    GSTAT_CRIT_RATING      = 10,  // melee + ranged + spell
    GSTAT_HASTE_RATING     = 11,  // melee + ranged + spell
    GSTAT_HIT_RATING       = 12,  // melee + ranged + spell
    GSTAT_DODGE_RATING     = 13,
    GSTAT_DEFENSE_RATING   = 14,
    GSTAT_PARRY_RATING     = 15,
    GSTAT_EXPERTISE_RATING = 16,
    GSTAT_MAX
};

enum AffixRollState : uint8
{
    AFFIX_ROLL_UNROLLED = 0,  // nothing was eligible when the item was rolled
    AFFIX_ROLL_APPLIED  = 2,
};

struct AffixDefinition
{
    uint32      id           = 0;
    std::string name;
    uint32      weight       = 0;
    uint8       statOp       = 0;
    uint8       roleMask     = 0;
    uint8       itemCategory = ITEM_CAT_ANY;
    uint8       minQuality   = 0;
    bool        requiresMana = false;
};

struct AffixSlot
{
    uint8  rollState   = AFFIX_ROLL_UNROLLED;
    uint32 affixId     = 0;
    int32  rolledValue = 0;
    bool   isCrit      = false;
};

struct ActiveStatMod
{
    uint8 statOp;
    int32 value;
};

// Per-player bookkeeping of the stat mods currently applied, keyed by item GUID.
struct ItemAffixPlayerData : public DataMap::Base
{
    std::unordered_map<uint64, std::vector<ActiveStatMod>> activeStatMods;
};

// Reforge candidate (also the serialized form "id:val:crit").
struct ReforgeOption
{
    uint32 affixId;
    int32  rolledValue;
    bool   isCrit;
};

struct ReforgeState
{
    bool   exists      = false;
    uint8  lockedSlot  = 0;
    uint32 rerollCount = 0;
};

enum class ReforgeRollResult : uint8
{
    OK = 0,
    ERR_NOT_APPLIED,
    ERR_WRONG_SLOT,
    ERR_INSUFFICIENT_GOLD,
};

enum class ReforgePickResult : uint8
{
    OK = 0,
    ERR_NO_PENDING,
    ERR_WRONG_SLOT,
    ERR_INVALID_INDEX,
};

class ItemAffixMgr
{
public:
    static ItemAffixMgr* instance();

    void LoadConfig();
    void LoadAffixTemplates();
    bool IsEnabled() const { return _enabled; }
    // Enabled and the player is not a playerbot (ItemAffixes.ExcludeBots).
    bool Handles(Player const* player) const;

    // Rolls every missing slot of an eligible item for this player's specialization.
    // No-op for bots, ineligible items and already-rolled items.
    void RollItem(Player* player, Item* item);

    // Rolls every eligible, never-rolled item the player has equipped (RollUnrolledOnEquip).
    void RollUnrolledEquipped(Player* player);

    // Follow the core's item stat lifecycle (Player::_ApplyItemMods -> OnPlayerAfterApplyItemMods):
    // equip, unequip, destroy, durability 0 and repair. Applying twice replaces the earlier mods.
    void ApplyItemAffixes(Player* player, Item* item);
    void RemoveItemAffixes(Player* player, Item* item);

    // Login: the core applies equipped items through _ApplyAllItemMods, which has no hook.
    // Removes every tracked affix stat and applies those of the equipped, usable items.
    void SyncAffixes(Player* player);

    // Deletes an item's affixes and rolls it again (GM command).
    void RerollItem(Player* player, Item* item);

    AffixDefinition const* GetAffixDef(uint32 id) const;
    std::vector<AffixSlot> GetSlots(uint64 itemGuid);
    std::string BuildAffixDisplayString(AffixDefinition const* def, int32 rolledValue) const;

    // Addon protocol (prefix AFXM).
    void HandleAddonMessage(Player* player, std::string const& payload);
    void SendAddonMsg(Player* player, std::string const& payload);
    void SendConfig(Player* player);
    void SendItemStatus(Player* player, Item* item);
    void RefreshAllItemStatus(Player* player);

    // Reforge
    ReforgeState GetReforgeState(uint64 itemGuid) const;
    uint32 GetReforgeCost(uint32 itemQuality, uint32 timesAlreadyReforged) const;
    ReforgeRollResult RollReforgeOptions(Player* player, Item* item, uint8 affixSlot,
                                         std::vector<ReforgeOption>* outOptions = nullptr);
    ReforgePickResult CommitReforgePick(Player* player, Item* item, uint8 affixSlot, uint32 optIdx);

private:
    struct RollContext;

    uint8 GetSlotCount(Item const* item) const;
    float GetAffixBudget(Item const* item) const;
    bool  IsEligibleItem(Item const* item) const;
    std::vector<std::pair<uint32, uint32>> BuildWeightedPool(RollContext const& ctx) const;
    uint32 PickAffix(RollContext const& ctx) const;
    void   RollValue(RollContext const& ctx, AffixDefinition const& def, int32& value, bool& isCrit) const;
    void   RefreshEquippedItem(Player* player, Item* item);
    void   StoreSlots(uint64 itemGuid, std::vector<AffixSlot> const& slots, uint8 firstChanged);
    void   AppendAffixPayload(std::string& msg, std::vector<AffixSlot> const& slots) const;

    bool  _enabled                   = true;
    bool  _excludeBots               = true;
    bool  _rollUnrolledOnEquip       = true;
    uint8 _slotCount[5]              = { 0, 0, 1, 2, 3 };  // indexed by quality 0..4
    uint8 _slotCountLegendary        = 4;
    uint8 _twoHanderBonusSlots       = 1;
    float _budgetFraction[5]         = { 0.f, 0.f, 0.18f, 0.13f, 0.10f };
    float _budgetFractionLegendary   = 0.09f;
    float _budgetMinRoll             = 0.75f;
    float _statMultiplier            = 1.5f;
    uint32 _maxBudgetItemLevel       = 100;
    uint32 _secondaryAttributeWeightPct = 33;
    bool  _critRollEnabled           = true;
    uint32 _critRollChance           = 10;
    uint8 _reforgeOptions[5]         = { 0, 0, 1, 2, 3 };
    uint8 _reforgeOptionsLegendary   = 4;
    uint32 _reforgeBaseCost[5]       = { 0, 0, 5000, 20000, 75000 };
    uint32 _reforgeBaseCostLegendary = 250000;

    std::unordered_map<uint32, AffixDefinition> _defs;

    // Item GUID -> slots. Authoritative once loaded; every write goes through StoreSlots.
    // Player hooks run on map update threads, so access is serialized.
    mutable std::mutex _cacheLock;
    std::unordered_map<uint64, std::vector<AffixSlot>> _cache;
};

#define sItemAffixMgr ItemAffixMgr::instance()

#endif
