-- ============================================================================
-- Reforge Master NPC (entry 601107, free in the CoA world DB as of 2026-11-23).
-- Appearance cloned from an enchanting trainer (display 18773).
-- Template only: spawn it with `.npc add 601107`; fixed town spawns come later.
-- ============================================================================

DELETE FROM `creature_template`       WHERE `entry`      = 601107;
DELETE FROM `creature_template_model` WHERE `CreatureID` = 601107;
DELETE FROM `npc_text`                WHERE `ID`         = 601107;
DELETE FROM `gossip_menu`             WHERE `MenuID`     = 601107;

INSERT INTO `creature_template`
    (`entry`, `name`, `subname`, `gossip_menu_id`, `minlevel`, `maxlevel`, `exp`,
     `faction`, `npcflag`, `speed_walk`, `speed_run`, `speed_swim`, `speed_flight`,
     `detection_range`, `unit_class`, `unit_flags`, `unit_flags2`, `type`,
     `HealthModifier`, `ManaModifier`, `ArmorModifier`, `ExperienceModifier`,
     `RegenHealth`, `flags_extra`, `ScriptName`)
VALUES
    (601107, 'Reforge Master', 'Affix Reforging', 601107, 60, 60, 0,
     35, 1, 1, 1.14286, 1, 1,
     20, 8, 33024, 2048, 7,
     1, 1, 1, 1,
     1, 2, 'npc_reforge_master');

INSERT INTO `creature_template_model`
    (`CreatureID`, `Idx`, `CreatureDisplayID`, `DisplayScale`, `Probability`)
VALUES
    (601107, 0, 18773, 1, 1);

INSERT INTO `npc_text` (`ID`, `text0_0`, `text0_1`, `Probability0`)
VALUES
    (601107,
     'Ah, another adventurer seeking to bend fate to their will. Bring me an item bearing affixes you would like reshaped, and for a fee I will let you see new possibilities within it.',
     'Ah, another adventurer seeking to bend fate to their will. Bring me an item bearing affixes you would like reshaped, and for a fee I will let you see new possibilities within it.',
     1);

INSERT INTO `gossip_menu` (`MenuID`, `TextID`)
VALUES
    (601107, 601107);
