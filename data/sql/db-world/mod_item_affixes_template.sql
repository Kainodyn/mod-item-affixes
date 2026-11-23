-- mod-item-affixes (CoA): stat affix pool. Idempotent: the table is rebuilt on every change of this file.
--
-- stat_op        GenericStatOp in src/ItemAffix.h
-- role_mask      0 = any spec; otherwise any of 1=CASTER 2=PHYSICAL 4=TANK 8=HEALER 16=RANGED must match
--                the looter's active specialization (src/CoASpecRoles.cpp)
-- item_category  0=any 1=1H weapon/shield 2=2H/ranged 3=any weapon 4=armor (incl. boots) 5=jewelry 6=wand 7=boots
-- min_quality    2=green+ 3=blue+ 4=epic+ (ItemQualities)
-- requires_mana  1 = only for characters with a mana pool
--
-- Strength/Agility/Intellect/Spirit are additionally gated by the specialization's primary stats
-- (ChrSpecs.dbc): the main attribute rolls at full weight, the spec's other listed attributes at
-- ItemAffixes.SecondaryAttributeWeightPct, and attributes the spec does not list never roll.
-- Removed for CoA: Hp5, Armor Penetration, Move Speed, Damage Reduction, all spellmod/talent/imprint/gem affixes.

DROP TABLE IF EXISTS `affix_template`;
CREATE TABLE `affix_template` (
  `id`            INT UNSIGNED     NOT NULL,
  `name`          VARCHAR(64)      NOT NULL,
  `weight`        INT UNSIGNED     NOT NULL DEFAULT 100 COMMENT '0 = disabled',
  `stat_op`       TINYINT UNSIGNED NOT NULL,
  `role_mask`     TINYINT UNSIGNED NOT NULL DEFAULT 0,
  `item_category` TINYINT UNSIGNED NOT NULL DEFAULT 0,
  `min_quality`   TINYINT UNSIGNED NOT NULL DEFAULT 2,
  `requires_mana` TINYINT UNSIGNED NOT NULL DEFAULT 0,
  PRIMARY KEY (`id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='mod-item-affixes: stat affix pool';

INSERT INTO `affix_template` (`id`, `name`, `weight`, `stat_op`, `role_mask`, `item_category`, `min_quality`, `requires_mana`) VALUES
-- Universal
(20000, 'Stamina',         120,  0,  0, 0, 2, 0),
(20001, 'Crit Rating',     100, 10,  0, 0, 2, 0),
(20002, 'Haste Rating',     90, 11,  0, 0, 2, 0),
(20003, 'Hit Rating',      100, 12,  0, 0, 2, 0),
-- Attributes (gated by the specialization's primary stats)
(20010, 'Strength',        150,  1,  0, 0, 2, 0),
(20020, 'Agility',         150,  2,  0, 0, 2, 0),
(20030, 'Intellect',       150,  3,  0, 0, 2, 0),
(20040, 'Spirit',          150,  4,  0, 0, 2, 0),
-- Caster / healer
(20100, 'Spell Power',      90,  7,  9, 0, 3, 0),
(20410, 'Mp5',              70,  8,  9, 0, 2, 1),
-- Physical
(20200, 'Attack Power',     90,  5,  2, 0, 3, 0),
(20220, 'Ranged AP',        90,  6, 16, 0, 3, 0),
(20300, 'Expertise',        80, 16,  2, 0, 3, 0),
-- Tank
(20430, 'Armor',            80,  9,  4, 4, 2, 0),
(20070, 'Dodge Rating',     80, 13,  4, 0, 3, 0),
(20080, 'Defense Rating',   80, 14,  4, 0, 3, 0),
(20090, 'Parry Rating',     80, 15,  4, 0, 3, 0);
