#ifndef MOD_ITEM_AFFIXES_ITEM_AFFIX_H
#define MOD_ITEM_AFFIXES_ITEM_AFFIX_H

#include "CoASpecRoles.h"
#include "DataMap.h"
#include "ObjectGuid.h"
#include "SpellDefines.h"
#include "Define.h"
#include <array>
#include <cstdint>
#include <mutex>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

class Creature;
class Item;
struct ItemTemplate;
class Player;
class Unit;
struct AscensionSpellRow;
struct SpellModifier;

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
// pet cooldown 22, pet damage taken 25, Max Health 27) and Armor Penetration (17).
// The summon stats (23, 24, 26) are percentages for every creature the player owns.
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
    GSTAT_MAX,  // end of the player stats

    GSTAT_SUMMON_HEALTH_PCT       = 23,
    GSTAT_SUMMON_DAMAGE_PCT       = 24,
    GSTAT_SUMMON_ATTACK_SPEED_PCT = 26,
};

inline bool IsSummonStat(uint8 statOp)
{
    return statOp == GSTAT_SUMMON_HEALTH_PCT || statOp == GSTAT_SUMMON_DAMAGE_PCT ||
        statOp == GSTAT_SUMMON_ATTACK_SPEED_PCT;
}

// Hidden passive aura put on the player's summons: effect 0 SPELL_AURA_MOD_INCREASE_HEALTH_PERCENT,
// effect 1 SPELL_AURA_MOD_MELEE_RANGED_HASTE (data/sql/db-world/mod_item_affixes_template_summon.sql).
constexpr uint32 SUMMON_AFFIX_CARRIER_SPELL = 9979002;

// Summed summon affix percentages of one player (before ItemAffixes.SummonAffixes.CapPct).
struct SummonStatTotals
{
    int32 healthPct      = 0;
    int32 damagePct      = 0;
    int32 attackSpeedPct = 0;
};

enum AffixRollState : uint8
{
    AFFIX_ROLL_UNROLLED = 0,  // nothing was eligible when the item was rolled
    AFFIX_ROLL_APPLIED  = 2,
};

// affix_template.affix_type
enum AffixType : uint8
{
    AFFIX_TYPE_SKILL = 0,  // CoA class skill affix (spell modifier)
    AFFIX_TYPE_STAT  = 1,
};

// Hidden passive dummy aura from data/sql/db-world/mod_item_affixes_template_skill.sql. Every skill
// SpellModifier is owned by it, so Player::ApplyModToSpell never sees a null ownerAura.
constexpr uint32 SKILL_AFFIX_CARRIER_SPELL = 9979001;
constexpr uint8  MAX_SKILL_AFFIX_EFFECTS   = 3;
// item_affix.rolled_value of a skill affix: percent of the template values (crit roll = 150).
constexpr int32  SKILL_AFFIX_BASE_SCALE    = 100;
constexpr int32  SKILL_AFFIX_CRIT_SCALE    = 150;

struct SkillAffixEffect
{
    uint8 op    = 0;      // SpellModOp
    bool  pct   = false;  // SPELLMOD_PCT, else SPELLMOD_FLAT
    int32 value = 0;
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

    // Skill affixes (affixType == AFFIX_TYPE_SKILL)
    uint8       affixType      = AFFIX_TYPE_STAT;
    std::string key;
    uint8       classId        = 0;
    uint16      specId         = 0;
    uint32      targetSpell    = 0;
    uint32      knownSpell     = 0;
    bool        scalesWithCrit = false;
    std::vector<SkillAffixEffect> effects;
    // affix_skill_target plus every other rank of those spells; one targetSpellId modifier each.
    std::vector<uint32> targets;

    bool IsSkill() const { return affixType == AFFIX_TYPE_SKILL; }
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

struct ActiveSkillAffix
{
    uint32 affixId;
    int32  scalePct;
};

// Summed skill affix modifiers of one spell, written into the Spell row streamed to the client.
// Indexed by SpellModOp.
constexpr uint8 SKILL_ROW_MAX_OPS = MAX_SPELLMOD;
struct SkillRowEffects
{
    std::array<int32, SKILL_ROW_MAX_OPS> flat{};
    std::array<int32, SKILL_ROW_MAX_OPS> pct{};
    std::vector<std::string> notes;  // affixes without a Spell row field (damage, crit, DoT, ...)
};

// Per-player bookkeeping of the stat mods currently applied, keyed by item GUID, and of the skill
// affix spell modifiers (owned here, never by the core) built from every applied skill affix.
struct ItemAffixPlayerData : public DataMap::Base
{
    ~ItemAffixPlayerData() override;

    std::unordered_map<uint64, std::vector<ActiveStatMod>>    activeStatMods;
    std::unordered_map<uint64, std::vector<ActiveSkillAffix>> activeSkillAffixes;
    std::vector<SpellModifier*>          skillMods;      // registered with Player::AddSpellMod
    std::unordered_map<uint32, SkillRowEffects> rowEffects;  // spell id -> modifiers its streamed row shows
    std::unordered_map<uint32, size_t>   sentRows;       // spell id -> signature of the modified row the client has
    bool   rowsReady          = true;    // false from login until the delayed first row sync
    bool   rowResetScheduled  = false;
    uint32 batchDepth         = 0;
    bool   rebuildPending     = false;
    bool   removingCarrier    = false;
    bool   rebuildScheduled   = false;
    bool   statusScheduled    = false;
    SummonStatTotals summonStats;
    bool   summonRefreshScheduled = false;
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

    // Summon affixes: totals kept per player, applied to every creature the player owns.
    void ApplySummonStat(Player* player, uint8 statOp, int32 value, bool apply);
    // Capped totals (ItemAffixes.SummonAffixes.CapPct); all zero for players the module does not handle.
    SummonStatTotals GetSummonStats(Player const* player) const;
    // The player a creature belongs to: pets, guardians, minions, totems and puppets (owner GUID).
    static Player* GetSummonOwner(Unit const* unit);
    // Sets the carrier aura of one summon to its owner's capped health and attack speed totals.
    void UpdateSummonCarrier(Creature* summon, Player* owner) const;
    // A summon entered the world: its carrier is set once its stats are initialised.
    void OnSummonAdded(Creature* summon, Player* owner);
    // Refreshes the carrier of every summon of the player in range (after equipment changes).
    void ScheduleSummonRefresh(Player* player);
    void RefreshSummons(Player* player);
    bool IsSummonSpec(uint32 specId) const { return _summonSpecs.count(specId) != 0; }
    // GM command: put a stat affix on an equipped item at the top of its value range.
    bool GiveStatAffix(Player* player, Item* item, AffixDefinition const& def);

    AffixDefinition const* GetAffixDef(uint32 id) const;
    AffixDefinition const* GetAffixDefByKey(std::string const& key) const;
    std::vector<AffixSlot> GetSlots(uint64 itemGuid);
    std::string BuildAffixDisplayString(AffixDefinition const* def, int32 rolledValue) const;

    // Skill affixes
    bool  PlayerKnowsSkillAffix(Player const* player, AffixDefinition const& def) const;
    // Rebuilds the carrier aura, spell modifiers and streamed rows from every applied skill affix.
    void  RebuildSkillMods(Player* player);
    // Removes and deletes every skill spell modifier (carrier aura removal, logout).
    void  ClearSkillMods(Player* player, ItemAffixPlayerData* data);
    void  OnCarrierRemoved(Player* player);
    void  ScheduleSkillStatusRefresh(Player* player);
    // GM command: put one skill affix on an equipped item (replaces its skill slot, else appends one).
    bool  GiveSkillAffix(Player* player, Item* item, AffixDefinition const& def, bool crit);
    size_t GetSkillAffixCount() const { return _skillAffixCount; }

    // Per-player streamed Spell rows (core patch core-ascension-spell-row-api). Sends the modified row
    // of every affected spell and the original row of every spell that is no longer affected.
    // resetClassSpells also sends the original row of every other spell the class's skill affixes can
    // target (login, spec change).
    void  SyncSpellRows(Player* player, bool resetClassSpells);
    void  ScheduleSpellRowReset(Player* player);
    static void ApplySpellRowEffects(SkillRowEffects const& effects, AscensionSpellRow& row);

    // Addon protocol (prefix AFXM).
    void HandleAddonMessage(Player* player, std::string const& payload);
    void SendAddonMsg(Player* player, std::string const& payload);
    void SendConfig(Player* player);
    void SendItemStatus(Player* player, Item* item);
    void RefreshAllItemStatus(Player* player);

    // Reforge
    ReforgeState GetReforgeState(uint64 itemGuid) const;
    // base(quality) * item level / ReforgeReferenceItemLevel * (1 + reforges so far * ReforgeCostIncreasePct / 100)
    uint32 GetReforgeCost(ItemTemplate const* proto, uint32 timesAlreadyReforged) const;
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
    uint32 PickSkillAffix(RollContext const& ctx) const;
    bool   RollsSkillAffix(RollContext const& ctx) const;
    void   RollValue(RollContext const& ctx, AffixDefinition const& def, int32& value, bool& isCrit) const;
    void   LoadSkillAffixes();
    void   BuildSkillMods(Player* player);
    void   SetBatch(Player* player, bool begin);
    bool   ItemHasSkillAffix(std::vector<AffixSlot> const& slots, size_t ignoreSlot = SIZE_MAX) const;
    void   RefreshEquippedItem(Player* player, Item* item);
    void   StoreSlots(uint64 itemGuid, std::vector<AffixSlot> const& slots, uint8 firstChanged);
    void   AppendAffixPayload(std::string& msg, std::vector<AffixSlot> const& slots, Player const* wearer) const;

    bool  _enabled                   = true;
    bool  _excludeBots               = true;
    bool  _rollUnrolledOnEquip       = true;
    uint8 _slotCount[5]              = { 0, 0, 1, 2, 3 };  // indexed by quality 0..4
    uint8 _slotCountLegendary        = 4;
    uint8 _twoHanderBonusSlots       = 1;
    uint32 _bonusSlotChance          = 5;
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
    uint32 _reforgeReferenceItemLevel = 60;
    uint32 _reforgeCostIncreasePct   = 50;
    bool   _skillAffixesEnabled      = true;
    uint32 _skillAffixChance         = 25;
    bool   _summonAffixesEnabled     = true;
    int32  _summonCapPct             = 100;
    std::unordered_set<uint32> _summonSpecs;

    std::unordered_map<uint32, AffixDefinition> _defs;
    std::unordered_map<std::string, uint32>     _defsByKey;
    std::unordered_map<uint8, std::vector<uint32>> _skillPoolByClass;
    size_t _skillAffixCount = 0;

    // Item GUID -> slots. Authoritative once loaded; every write goes through StoreSlots.
    // Player hooks run on map update threads, so access is serialized.
    mutable std::mutex _cacheLock;
    std::unordered_map<uint64, std::vector<AffixSlot>> _cache;
};

#define sItemAffixMgr ItemAffixMgr::instance()

#endif
