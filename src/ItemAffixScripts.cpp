#include "ItemAffix.h"

#include "Item.h"
#include "Player.h"
#include "ScriptMgr.h"
#include "SharedDefines.h"
#include "SpellAuras.h"

class ItemAffixWorldScript : public WorldScript
{
public:
    ItemAffixWorldScript() : WorldScript("ItemAffixWorldScript", { WORLDHOOK_ON_BEFORE_WORLD_INITIALIZED }) { }

    void OnBeforeWorldInitialized() override
    {
        sItemAffixMgr->LoadConfig();
        CoASpecRoles::Load();
        sItemAffixMgr->LoadAffixTemplates();
    }
};

class ItemAffixPlayerScript : public PlayerScript
{
public:
    ItemAffixPlayerScript() : PlayerScript("ItemAffixPlayerScript", {
        PLAYERHOOK_ON_LOGIN,
        PLAYERHOOK_ON_STORE_NEW_ITEM,
        PLAYERHOOK_ON_AFTER_STORE_OR_EQUIP_NEW_ITEM,
        PLAYERHOOK_ON_EQUIP,
        PLAYERHOOK_ON_AFTER_APPLY_ITEM_MODS,
        PLAYERHOOK_ON_BEFORE_SEND_CHAT_MESSAGE,
        PLAYERHOOK_ON_LOGOUT,
        PLAYERHOOK_ON_LEARN_SPELL,
        PLAYERHOOK_ON_FORGOT_SPELL,
        PLAYERHOOK_ON_AFTER_SPEC_SLOT_CHANGED,
    }) { }

    void OnPlayerLogin(Player* player) override
    {
        if (!sItemAffixMgr->Handles(player))
            return;
        sItemAffixMgr->ScheduleSpellRowReset(player);
        sItemAffixMgr->RollUnrolledEquipped(player);
        sItemAffixMgr->SyncAffixes(player);
        sItemAffixMgr->SendConfig(player);
    }

    // Loot, quest rewards, crafting, vendor purchases.
    void OnPlayerStoreNewItem(Player* player, Item* item, uint32 /*count*/) override
    {
        sItemAffixMgr->RollItem(player, item);
    }

    void OnPlayerAfterStoreOrEquipNewItem(Player* player, uint32 /*vendorslot*/, Item* item, uint8 /*count*/,
        uint8 /*bag*/, uint8 /*slot*/, ItemTemplate const* /*proto*/, Creature* /*vendor*/,
        VendorItem const* /*crItem*/, bool /*bStore*/) override
    {
        sItemAffixMgr->RollItem(player, item);
    }

    // Items that never passed through a store hook (mail, auction, trade, gear from before the
    // module) roll for the wearer when first equipped (ItemAffixes.RollUnrolledOnEquip).
    void OnPlayerEquip(Player* player, Item* item, uint8 /*bag*/, uint8 /*slot*/, bool /*update*/) override
    {
        if (!item || !sItemAffixMgr->Handles(player))
            return;
        sItemAffixMgr->RollUnrolledEquipped(player);
        sItemAffixMgr->SendItemStatus(player, item);
    }

    // Player::_ApplyItemMods: equip, unequip, destroy, durability 0 and repair.
    void OnPlayerAfterApplyItemMods(Player* player, Item* item, uint8 /*slot*/, bool apply) override
    {
        if (apply)
            sItemAffixMgr->ApplyItemAffixes(player, item);
        else
            sItemAffixMgr->RemoveItemAffixes(player, item);
    }

    // Skill modifiers go before the carrier aura, which CleanupsBeforeDelete removes afterwards.
    void OnPlayerLogout(Player* player) override
    {
        ItemAffixPlayerData* data = player->CustomData.Get<ItemAffixPlayerData>("ItemAffixData");
        if (!data)
            return;
        data->activeSkillAffixes.clear();
        data->rowEffects.clear();
        data->sentRows.clear();
        sItemAffixMgr->ClearSkillMods(player, data);
    }

    void OnPlayerLearnSpell(Player* player, uint32 /*spellID*/) override
    {
        sItemAffixMgr->ScheduleSkillStatusRefresh(player);
    }

    void OnPlayerForgotSpell(Player* player, uint32 /*spellID*/) override
    {
        sItemAffixMgr->ScheduleSkillStatusRefresh(player);
    }

    void OnPlayerAfterSpecSlotChanged(Player* player, uint8 /*newSlot*/) override
    {
        sItemAffixMgr->ScheduleSpellRowReset(player);
    }

    // AFXM requests arrive as LANG_ADDON whispers to the player themself ("AFXM\t<payload>").
    // Clearing the text stops the request echoing back to the addon without the core patch
    // upstream needed: an empty addon whisper reaches the client with an empty prefix.
    void OnPlayerBeforeSendChatMessage(Player* player, uint32& type, uint32& lang, std::string& msg) override
    {
        if (lang != LANG_ADDON || type != CHAT_MSG_WHISPER || !sItemAffixMgr->Handles(player))
            return;
        if (msg.size() < 5 || msg.compare(0, 5, "AFXM\t") != 0)
            return;

        sItemAffixMgr->HandleAddonMessage(player, msg.substr(5));
        msg.clear();
    }
};

class ItemAffixUnitScript : public UnitScript
{
public:
    ItemAffixUnitScript() : UnitScript("ItemAffixUnitScript", true, { UNITHOOK_ON_AURA_REMOVE }) { }

    void OnAuraRemove(Unit* unit, AuraApplication* aurApp, AuraRemoveMode /*mode*/) override
    {
        if (!aurApp || aurApp->GetBase()->GetId() != SKILL_AFFIX_CARRIER_SPELL)
            return;
        if (Player* player = unit ? unit->ToPlayer() : nullptr)
            sItemAffixMgr->OnCarrierRemoved(player);
    }
};

void AddSC_item_affix_scripts()
{
    new ItemAffixWorldScript();
    new ItemAffixPlayerScript();
    new ItemAffixUnitScript();
}
