#!/usr/bin/env python3
"""
Linux/macOS port of scripts/apply_core_patches.ps1 (same patches, same
detect/search/replace text, idempotent). Keep the two in sync.

    python3 scripts/apply_core_patches.py /path/to/azerothcore [--check]

--check only reports what would change and exits 1 if a patch cannot apply.
"""
import argparse
import os
import sys

PATCHES = [
    ("Patch 1: Player::ApplyModToSpell null-guard (ownerAura)",
     "src/server/game/Entities/Player/Player.cpp",
     "ownerAura is null for item-affix mods",
     """    // don't do anything with no charges
    if (mod->ownerAura->IsUsingCharges() && !mod->ownerAura->GetCharges())
        return;

    // register inside spell, proc system uses this to drop charges
    spell->m_appliedMods.insert(mod->ownerAura);
""",
     """    // don't do anything with no charges (ownerAura is null for item-affix mods - skip charge logic)
    if (mod->ownerAura && mod->ownerAura->IsUsingCharges() && !mod->ownerAura->GetCharges())
        return;

    // register inside spell, proc system uses this to drop charges; skip if no ownerAura (item-affix mods have none)
    if (mod->ownerAura)
        spell->m_appliedMods.insert(mod->ownerAura);
"""),
    ("Patch 2a: PlayerHook enum value PLAYERHOOK_ON_SOCKET_GEM",
     "src/server/game/Scripting/ScriptDefines/PlayerScript.h",
     "PLAYERHOOK_ON_SOCKET_GEM",
     "    PLAYERHOOK_ON_UNEQUIP_ITEM,\n",
     "    PLAYERHOOK_ON_UNEQUIP_ITEM,\n    PLAYERHOOK_ON_SOCKET_GEM,\n"),
    ("Patch 2b: PlayerScript virtual OnPlayerSocketGem method",
     "src/server/game/Scripting/ScriptDefines/PlayerScript.h",
     "virtual void OnPlayerSocketGem",
     """    // After an item has been unequipped
    virtual void OnPlayerUnequip(Player* /*player*/, Item* /*it*/) { }
""",
     """    // After an item has been unequipped
    virtual void OnPlayerUnequip(Player* /*player*/, Item* /*it*/) { }

    // After a gem is socketed into an item (before the gem item is destroyed)
    virtual void OnPlayerSocketGem(Player* /*player*/, Item* /*item*/, Item* /*gem*/, uint8 /*slot*/) { }
"""),
    ("Patch 2c: ScriptMgr OnPlayerSocketGem dispatcher",
     "src/server/game/Scripting/ScriptDefines/PlayerScript.cpp",
     "ScriptMgr::OnPlayerSocketGem",
     "template class AC_GAME_API ScriptRegistry<PlayerScript>;",
     """void ScriptMgr::OnPlayerSocketGem(Player* player, Item* item, Item* gem, uint8 slot)
{
    CALL_ENABLED_HOOKS(PlayerScript, PLAYERHOOK_ON_SOCKET_GEM, script->OnPlayerSocketGem(player, item, gem, slot));
}

template class AC_GAME_API ScriptRegistry<PlayerScript>;"""),
    ("Patch 2d: ScriptMgr.h OnPlayerSocketGem declaration",
     "src/server/game/Scripting/ScriptMgr.h",
     "void OnPlayerSocketGem(",
     "    void OnPlayerUnequip(Player* player, Item* it);",
     "    void OnPlayerUnequip(Player* player, Item* it);\n    void OnPlayerSocketGem(Player* player, Item* item, Item* gem, uint8 slot);"),
    ("Patch 2e: HandleSocketOpcode OnPlayerSocketGem callback (gem affixes)",
     "src/server/game/Handlers/ItemHandler.cpp",
     "OnPlayerSocketGem",
     """            if (Item* guidItem = _player->GetItemByGuid(packet.GemGuids[i]))
                _player->DestroyItem(guidItem->GetBagSlot(), guidItem->GetSlot(), true);
""",
     """            if (Item* guidItem = _player->GetItemByGuid(packet.GemGuids[i]))
            {
                sScriptMgr->OnPlayerSocketGem(_player, itemTarget, guidItem, i);
                _player->DestroyItem(guidItem->GetBagSlot(), guidItem->GetSlot(), true);
            }
"""),
    ("Patch 3: Unit::DealDamage - player-owned summon damage counts as player damage (loot/XP)",
     "src/server/game/Entities/Unit/Unit.cpp",
     "attacker->GetOwnerGUID().IsPlayer()",
     """            bool damagedByPlayer = unDamage && attacker && (attacker->IsPlayer() || attacker->m_movedByPlayer != nullptr
                || attacker->GetCharmerGUID().IsPlayer());
""",
     """            bool damagedByPlayer = unDamage && attacker && (attacker->IsPlayer() || attacker->m_movedByPlayer != nullptr
                || attacker->GetCharmerGUID().IsPlayer() || attacker->GetOwnerGUID().IsPlayer());
"""),
    ("Patch 4: Unit::EngageWithTarget - player-owned summons tap mob on engage (loot recipient)",
     "src/server/game/Entities/Unit/Unit.cpp",
     "IsPlayer() || IsControlledByPlayer()",
     """    if (Creature* creature = target->ToCreature())
        if (!creature->hasLootRecipient() && IsPlayer())
            creature->SetLootRecipient(this);
""",
     """    if (Creature* creature = target->ToCreature())
        if (!creature->hasLootRecipient() && (IsPlayer() || IsControlledByPlayer()))
            creature->SetLootRecipient(this);
"""),
    ("Patch 5: ChatHandler - suppress addon-message suppression-sentinel log spam",
     "src/server/game/Handlers/ChatHandler.cpp",
     "message handled, suppress delivery",
     """    sScriptMgr->OnPlayerBeforeSendChatMessage(_player, type, lang, msg);

    switch (type)
""",
     """    sScriptMgr->OnPlayerBeforeSendChatMessage(_player, type, lang, msg);

    // Some modules (e.g. mod-item-affixes) use type=0 + lang=LANG_ADDON as a
    // "message handled, suppress delivery" sentinel via OnPlayerBeforeSendChatMessage.
    // Catch it here rather than falling through to the switch's default case, which
    // would otherwise log a spurious "unknown message type" error every time.
    if (type == 0 && lang == LANG_ADDON)
    {
        return;
    }

    switch (type)
"""),
    ("Patch 6a: PlayerHook enum value PLAYERHOOK_ON_BEFORE_QUEST_REWARD",
     "src/server/game/Scripting/ScriptDefines/PlayerScript.h",
     "PLAYERHOOK_ON_BEFORE_QUEST_REWARD",
     "    PLAYERHOOK_ON_SOCKET_GEM,\n",
     "    PLAYERHOOK_ON_SOCKET_GEM,\n    PLAYERHOOK_ON_BEFORE_QUEST_REWARD,\n"),
    ("Patch 6b: PlayerScript virtual OnPlayerBeforeQuestReward method",
     "src/server/game/Scripting/ScriptDefines/PlayerScript.h",
     "virtual void OnPlayerBeforeQuestReward",
     """    // After a gem is socketed into an item (before the gem item is destroyed)
    virtual void OnPlayerSocketGem(Player* /*player*/, Item* /*item*/, Item* /*gem*/, uint8 /*slot*/) { }
""",
     """    // After a gem is socketed into an item (before the gem item is destroyed)
    virtual void OnPlayerSocketGem(Player* /*player*/, Item* /*item*/, Item* /*gem*/, uint8 /*slot*/) { }

    // Just before RewardQuest stores any reward item(s) -- fires once per quest
    // turn-in, before both reward-item loops run. Lets a module distinguish a
    // quest-reward item from any other newly-stored item at the point its own
    // OnPlayerStoreNewItem/OnPlayerAfterStoreOrEquipNewItem hook fires for it.
    virtual void OnPlayerBeforeQuestReward(Player* /*player*/, Quest const* /*quest*/) { }
"""),
    ("Patch 6c: ScriptMgr OnPlayerBeforeQuestReward dispatcher",
     "src/server/game/Scripting/ScriptDefines/PlayerScript.cpp",
     "ScriptMgr::OnPlayerBeforeQuestReward",
     "template class AC_GAME_API ScriptRegistry<PlayerScript>;",
     """void ScriptMgr::OnPlayerBeforeQuestReward(Player* player, Quest const* quest)
{
    CALL_ENABLED_HOOKS(PlayerScript, PLAYERHOOK_ON_BEFORE_QUEST_REWARD, script->OnPlayerBeforeQuestReward(player, quest));
}

template class AC_GAME_API ScriptRegistry<PlayerScript>;"""),
    ("Patch 6d: ScriptMgr.h OnPlayerBeforeQuestReward declaration",
     "src/server/game/Scripting/ScriptMgr.h",
     "void OnPlayerBeforeQuestReward(",
     "    void OnPlayerSocketGem(Player* player, Item* item, Item* gem, uint8 slot);",
     "    void OnPlayerSocketGem(Player* player, Item* item, Item* gem, uint8 slot);\n    void OnPlayerBeforeQuestReward(Player* player, Quest const* quest);"),
    ("Patch 6e: Player::RewardQuest OnPlayerBeforeQuestReward call site",
     "src/server/game/Entities/Player/PlayerQuest.cpp",
     "OnPlayerBeforeQuestReward",
     """void Player::RewardQuest(Quest const* quest, uint32 reward, Object* questGiver, bool announce, bool isLFGReward)
{
    //this THING should be here to protect code from quest, which cast on player far teleport as a reward
    //should work fine, cause far teleport will be executed in Player::Update()
    SetMustDelayTeleport(true);
""",
     """void Player::RewardQuest(Quest const* quest, uint32 reward, Object* questGiver, bool announce, bool isLFGReward)
{
    //this THING should be here to protect code from quest, which cast on player far teleport as a reward
    //should work fine, cause far teleport will be executed in Player::Update()
    SetMustDelayTeleport(true);

    // Fires before any reward item is stored, so a module's own
    // OnPlayerStoreNewItem/OnPlayerAfterStoreOrEquipNewItem handler can tell
    // a quest-reward item apart from any other newly-stored item.
    sScriptMgr->OnPlayerBeforeQuestReward(this, quest);
"""),
]


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("root", nargs="?",
                    default=os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", "..")))
    ap.add_argument("--check", action="store_true")
    args = ap.parse_args()

    failed = False
    for desc, rel, detect, search, replace in PATCHES:
        path = os.path.join(args.root, rel)
        if not os.path.isfile(path):
            print(f"  [FAIL] {desc}: file not found: {path}")
            failed = True
            continue
        with open(path, encoding="utf-8", newline="") as fh:
            raw = fh.read()
        crlf = "\r\n" in raw
        content = raw.replace("\r\n", "\n")
        if detect in content:
            print(f"  [--]   {desc} (already applied)")
            continue
        if search not in content:
            print(f"  [FAIL] {desc}: search text not found in {rel} (see CORE_PATCHES.md)")
            failed = True
            continue
        if args.check:
            print(f"  [ok?]  {desc} (would apply)")
            continue
        patched = content.replace(search, replace, 1)
        if crlf:
            patched = patched.replace("\n", "\r\n")
        with open(path, "w", encoding="utf-8", newline="") as fh:
            fh.write(patched)
        print(f"  [OK]   {desc}")
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())
