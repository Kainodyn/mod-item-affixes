-- mod-item-affixes (CoA): per-item affix state. Applied to every realm's characters database
-- (acore_characters and acore_characters_wc) by the DB updater. Idempotent.

CREATE TABLE IF NOT EXISTS `item_affix` (
  `item_guid`         BIGINT UNSIGNED  NOT NULL COMMENT 'ObjectGuid raw value of the item',
  `affix_slot`        TINYINT UNSIGNED NOT NULL COMMENT '0-based slot index',
  `affix_id`          INT UNSIGNED     NOT NULL DEFAULT 0 COMMENT 'affix_template.id; 0 = nothing eligible',
  `rolled_value`      INT              NOT NULL DEFAULT 0 COMMENT 'Stat magnitude',
  `roll_state`        TINYINT UNSIGNED NOT NULL DEFAULT 0 COMMENT '0=UNROLLED, 2=APPLIED (1=PENDING is upstream manual mode, unused)',
  `pending_opts`      VARCHAR(255)     NOT NULL DEFAULT '' COMMENT 'Unused in CoA (upstream manual mode)',
  `rerolls_remaining` TINYINT UNSIGNED NOT NULL DEFAULT 0 COMMENT 'Unused in CoA (upstream manual mode)',
  `locked_mask`       TINYINT UNSIGNED NOT NULL DEFAULT 0 COMMENT 'Unused in CoA (upstream manual mode)',
  `pending_spec`      TINYINT          NOT NULL DEFAULT -1 COMMENT 'Unused in CoA (upstream manual mode)',
  `is_crit`           TINYINT UNSIGNED NOT NULL DEFAULT 0 COMMENT '1 = value came from a crit roll',
  PRIMARY KEY (`item_guid`, `affix_slot`),
  KEY `idx_affix_id` (`affix_id`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='mod-item-affixes: affixes keyed by item GUID';

CREATE TABLE IF NOT EXISTS `item_reforge_state` (
  `item_guid`    BIGINT UNSIGNED  NOT NULL,
  `locked_slot`  TINYINT UNSIGNED NOT NULL COMMENT 'Slot the item is locked to after its first reforge',
  `reroll_count` INT UNSIGNED     NOT NULL DEFAULT 1 COMMENT 'Times reforged; drives the escalating cost',
  `pending_opts` VARCHAR(255)     NOT NULL DEFAULT '' COMMENT 'Server-generated candidates "id:val:crit,..." awaiting a pick',
  PRIMARY KEY (`item_guid`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='mod-item-affixes: Reforge lock-in state';
