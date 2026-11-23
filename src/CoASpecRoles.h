#ifndef MOD_ITEM_AFFIXES_COA_SPEC_ROLES_H
#define MOD_ITEM_AFFIXES_COA_SPEC_ROLES_H

#include "Define.h"
#include <array>

class Player;

// Role bits shared with affix_template.role_mask.
enum AffixRoleGroup : uint8
{
    AFFIX_ROLE_ANY      = 0,
    AFFIX_ROLE_CASTER   = 1,
    AFFIX_ROLE_PHYSICAL = 2,
    AFFIX_ROLE_TANK     = 4,
    AFFIX_ROLE_HEALER   = 8,
    AFFIX_ROLE_RANGED   = 16,  // physical ranged (uses Ranged Attack Power)
};

// What the affix roller needs to know about the looter's specialization.
struct AffixSpecProfile
{
    uint32 specId   = 0;      // ChrSpecs.dbc id; 0 = none resolved
    uint8  roleMask = 0;      // AffixRoleGroup bits; 0 = no specialization -> universal affixes only
    bool   anyRole  = false;  // Hero (spec 94): every role and attribute is eligible
    // ItemMods (ITEM_MOD_AGILITY, ...) from ChrSpecs.dbc PrimaryStats, in order; [0] is the main stat.
    std::array<uint8, 3> primaryStats{};
};

namespace CoASpecRoles
{
    // Reads ChrSpecs.dbc from the server's client DBC directory. Called once at startup.
    void Load();

    // The player's active specialization as an affix profile. CoA classes (12-32) use
    // GetAscensionActiveSpecialization; the original classes map their dominant talent tree
    // onto ChrSpecs 64-93.
    AffixSpecProfile Resolve(Player* player);
}

#endif
