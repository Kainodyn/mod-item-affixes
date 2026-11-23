#include "CoASpecRoles.h"

#include "AscensionBossControlRolesPolicy.h"
#include "AscensionSpecialization.h"
#include "ClientDBC.h"
#include "DBCStores.h"
#include "ItemTemplate.h"
#include "Log.h"
#include "Player.h"
#include <algorithm>
#include <string_view>
#include <unordered_map>

namespace
{
// ChrSpecs.dbc columns (DWORD index), verified against the CoA client set on 2026-11-23.
enum ChrSpecsField : uint32
{
    CHR_SPECS_ID           = 0,
    CHR_SPECS_PRIMARY_STAT = 8,   // 3 strings: "Agility", "Strength", "Intellect", "Spirit", "Stamina", "None"
    CHR_SPECS_ROLE_MELEE   = 11,
    CHR_SPECS_ROLE_RANGED  = 12,
    CHR_SPECS_ROLE_CASTER  = 13,
    CHR_SPECS_ROLE_TANK    = 14,
    CHR_SPECS_ROLE_HEALER  = 15,
    CHR_SPECS_MIN_DWORDS   = 16,
};

constexpr uint32 SPEC_HERO = 94;

struct SpecData
{
    bool melee = false, ranged = false, caster = false, tank = false, healer = false;
    std::array<uint8, 3> primaryStats{};
};

std::unordered_map<uint32, SpecData> SpecsById;

uint8 StatByName(std::string_view name)
{
    if (name == "Agility")   return ITEM_MOD_AGILITY;
    if (name == "Strength")  return ITEM_MOD_STRENGTH;
    if (name == "Intellect") return ITEM_MOD_INTELLECT;
    if (name == "Spirit")    return ITEM_MOD_SPIRIT;
    if (name == "Stamina")   return ITEM_MOD_STAMINA;
    return 0;
}

bool IsPhysicalAttribute(uint8 stat) { return stat == ITEM_MOD_STRENGTH || stat == ITEM_MOD_AGILITY; }
bool IsCasterAttribute(uint8 stat)   { return stat == ITEM_MOD_INTELLECT || stat == ITEM_MOD_SPIRIT; }

// Original classes have no CoA specialization state; their ChrSpecs rows (64-93) follow the
// talent tab order, except the Warlock, whose rows are Affliction, Destruction, Demonology.
uint32 OriginalClassSpec(uint8 classId, int tree)
{
    static std::unordered_map<uint8, std::array<uint32, 3>> const specsByClass = {
        { CLASS_WARRIOR,      { 64, 65, 66 } },
        { CLASS_PALADIN,      { 67, 68, 69 } },
        { CLASS_HUNTER,       { 70, 71, 72 } },
        { CLASS_ROGUE,        { 73, 74, 75 } },
        { CLASS_PRIEST,       { 76, 77, 78 } },
        { CLASS_DEATH_KNIGHT, { 79, 80, 81 } },
        { CLASS_SHAMAN,       { 82, 83, 84 } },
        { CLASS_MAGE,         { 85, 86, 87 } },
        { CLASS_WARLOCK,      { 88, 90, 89 } },
        { CLASS_DRUID,        { 91, 92, 93 } },
    };
    auto it = specsByClass.find(classId);
    return (it != specsByClass.end() && tree >= 0 && tree < 3) ? it->second[tree] : 0;
}

int DominantTalentTree(Player* player)
{
    uint32 const* tabPages = GetTalentTabPages(player->getClass());
    if (!tabPages)
        return -1;

    int counts[3] = { 0, 0, 0 };
    for (auto const& [spellId, talent] : player->GetTalentMap())
    {
        if (!talent || talent->State == PLAYERSPELL_REMOVED || !talent->IsInSpec(player->GetActiveSpec()))
            continue;
        TalentSpellPos const* pos = GetTalentSpellPos(spellId);
        if (!pos)
            continue;
        TalentEntry const* entry = sTalentStore.LookupEntry(pos->talent_id);
        if (!entry)
            continue;
        for (int t = 0; t < 3; ++t)
            if (entry->TalentTab == tabPages[t])
            {
                counts[t] += pos->rank + 1;
                break;
            }
    }

    int best = 0;
    for (int i = 1; i < 3; ++i)
        if (counts[i] > counts[best])
            best = i;
    return counts[best] > 0 ? best : -1;
}

uint32 ActiveSpec(Player* player)
{
    if (player->getClass() == CLASS_HERO)
        return SPEC_HERO;
    if (IsAscensionCustomClassId(player->getClass()))
        return GetAscensionActiveSpecialization(player);
    return OriginalClassSpec(player->getClass(), DominantTalentTree(player));
}
}

void CoASpecRoles::Load()
{
    SpecsById.clear();

    ClientDBC specs;
    if (!specs.Load(GetClientDBCPath("ChrSpecs.dbc"), CHR_SPECS_MIN_DWORDS))
    {
        LOG_ERROR("module", "mod-item-affixes: ChrSpecs.dbc not readable; every character rolls universal affixes only.");
        return;
    }

    for (uint32 row = 0; row < specs.GetRecordCount(); ++row)
    {
        ClientDBC::Record record = specs.GetRecord(row);
        uint32 const specId = record.GetUInt32(CHR_SPECS_ID);

        SpecData data;
        data.melee  = record.GetUInt32(CHR_SPECS_ROLE_MELEE) != 0;
        data.ranged = record.GetUInt32(CHR_SPECS_ROLE_RANGED) != 0;
        data.caster = record.GetUInt32(CHR_SPECS_ROLE_CASTER) != 0;
        // CoA's curated tank/healer lists (boss control roles) win where the DBC flags are incomplete,
        // e.g. Fleshweaver (25) and Grovekeeper (58) heal without the DBC healer flag.
        data.tank   = record.GetUInt32(CHR_SPECS_ROLE_TANK) != 0 ||
            std::find(BossControlRoles::TankSpecializations.begin(), BossControlRoles::TankSpecializations.end(),
                specId) != BossControlRoles::TankSpecializations.end();
        data.healer = record.GetUInt32(CHR_SPECS_ROLE_HEALER) != 0 ||
            std::find(BossControlRoles::HealerSpecializations.begin(), BossControlRoles::HealerSpecializations.end(),
                specId) != BossControlRoles::HealerSpecializations.end();

        uint8 index = 0;
        for (uint32 field = 0; field < 3; ++field)
            if (uint8 stat = StatByName(record.GetString(CHR_SPECS_PRIMARY_STAT + field)))
                data.primaryStats[index++] = stat;

        SpecsById[specId] = data;
    }

    LOG_INFO("module", "mod-item-affixes: loaded {} specialization role profile(s) from ChrSpecs.dbc.", SpecsById.size());
}

AffixSpecProfile CoASpecRoles::Resolve(Player* player)
{
    AffixSpecProfile profile;
    if (!player)
        return profile;

    profile.specId = ActiveSpec(player);
    if (profile.specId == SPEC_HERO)
    {
        profile.anyRole = true;
        return profile;
    }

    auto it = SpecsById.find(profile.specId);
    if (it == SpecsById.end())
    {
        profile.specId = 0;
        return profile;
    }

    SpecData const& spec = it->second;
    profile.primaryStats = spec.primaryStats;

    bool physicalAttribute = false, casterAttribute = false;
    for (uint8 stat : spec.primaryStats)
    {
        physicalAttribute |= IsPhysicalAttribute(stat);
        casterAttribute   |= IsCasterAttribute(stat);
    }

    uint8 roles = 0;
    if (spec.melee)                       roles |= AFFIX_ROLE_PHYSICAL;
    if (spec.ranged && physicalAttribute) roles |= AFFIX_ROLE_RANGED;
    if (spec.ranged && !physicalAttribute) roles |= AFFIX_ROLE_CASTER;  // e.g. Intellect-based ranged specs
    if (spec.caster)                      roles |= AFFIX_ROLE_CASTER;
    if (spec.healer)                      roles |= AFFIX_ROLE_HEALER;
    if (spec.tank)
    {
        roles |= AFFIX_ROLE_TANK;
        // A tank's offensive stats follow its attributes (Sanguine and Moon Guard tank with Spirit/Intellect).
        if (physicalAttribute) roles |= AFFIX_ROLE_PHYSICAL;
        if (casterAttribute)   roles |= AFFIX_ROLE_CASTER;
    }
    // A spec without any role flag still gets the stats its attributes imply.
    if (!roles)
        roles = (physicalAttribute ? AFFIX_ROLE_PHYSICAL : 0) | (casterAttribute ? AFFIX_ROLE_CASTER : 0);

    profile.roleMask = roles;
    return profile;
}
