-- mod-item-affixes (CoA): summon stat affixes. Re-applied by the DB updater whenever it changes, so every
-- statement is idempotent. Sorts after mod_item_affixes_template.sql (which recreates affix_template) and
-- mod_item_affixes_template_skill.sql: if the first changes, re-apply this one too (delete its row from `updates`).
--
-- stat_op  23 = Summon Health %, 24 = Summon Damage %, 26 = Summon Attack Speed % (GenericStatOp in src/ItemAffix.h).
-- Rolled only for the specializations in ItemAffixes.SummonAffixes.Specs, at most one per item.

DELETE FROM `affix_template` WHERE `id` IN (20500, 20501, 20502);
INSERT INTO `affix_template` (`id`, `name`, `weight`, `stat_op`, `role_mask`, `item_category`, `min_quality`, `requires_mana`) VALUES
(20500, 'Summon Damage',       100, 24, 0, 0, 2, 0),
(20501, 'Summon Health',        80, 23, 0, 0, 2, 0),
(20502, 'Summon Attack Speed',  60, 26, 0, 0, 2, 0);

-- Hidden passive carrier on each summon: effect 1 SPELL_AURA_MOD_INCREASE_HEALTH_PERCENT (133), effect 2
-- SPELL_AURA_MOD_MELEE_RANGED_HASTE (192). Amounts are set by the module (0 in the DBC row).
DELETE FROM `spell_dbc` WHERE `ID` = 9979002;
INSERT INTO `spell_dbc` (`ID`, `Attributes`, `CastingTimeIndex`, `DurationIndex`, `RangeIndex`, `EquippedItemClass`, `Effect_1`, `ImplicitTargetA_1`, `EffectAura_1`, `Effect_2`, `ImplicitTargetA_2`, `EffectAura_2`, `Name_Lang_enUS`, `Name_Lang_Mask`, `SchoolMask`) VALUES
(9979002, 448, 1, 21, 1, -1, 6, 1, 133, 6, 1, 192, 'Item Affix Summon Carrier', 16712190, 1);
