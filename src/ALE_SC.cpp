/*
 * This file is part of the AzerothCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "Chat.h"
#include "ALEEventMgr.h"
#include "Log.h"
#include "LuaEngine.h"
#include "Pet.h"
#include "Player.h"
#include "ScriptHookSubscription.h"
#include "ScriptMgr.h"
#include "ScriptedGossip.h"

class ALE_AllCreatureScript : public AllCreatureScript
{
public:
    ALE_AllCreatureScript() : AllCreatureScript("ALE_AllCreatureScript") { }

    // Creature
    bool CanCreatureGossipHello(Player* player, Creature* creature) override
    {
        if (sALE->OnGossipHello(player, creature))
            return true;

        return false;
    }

    bool CanCreatureGossipSelect(Player* player, Creature* creature, uint32 sender, uint32 action) override
    {
        if (sALE->OnGossipSelect(player, creature, sender, action))
            return true;

        return false;
    }

    bool CanCreatureGossipSelectCode(Player* player, Creature* creature, uint32 sender, uint32 action, const char* code) override
    {
        if (sALE->OnGossipSelectCode(player, creature, sender, action, code))
            return true;

        return false;
    }

    void OnCreatureAddWorld(Creature* creature) override
    {
        sALE->OnAddToWorld(creature);
        sALE->OnAllCreatureAddToWorld(creature);

        if (creature->IsGuardian() && creature->ToTempSummon() && creature->ToTempSummon()->GetSummonerGUID().IsPlayer())
            sALE->OnPetAddedToWorld(creature->ToTempSummon()->GetSummonerUnit()->ToPlayer(), creature);
    }

    void OnCreatureRemoveWorld(Creature* creature) override
    {
        sALE->OnRemoveFromWorld(creature);
        sALE->OnAllCreatureRemoveFromWorld(creature);
    }

    bool CanCreatureQuestAccept(Player* player, Creature* creature, Quest const* quest) override
    {
        sALE->OnPlayerQuestAccept(player, quest);
        sALE->OnQuestAccept(player, creature, quest);
        return false;
    }

    bool CanCreatureQuestReward(Player* player, Creature* creature, Quest const* quest, uint32 opt) override
    {
        if (sALE->OnQuestReward(player, creature, quest, opt))
        {
            ClearGossipMenuFor(player);
            return true;
        }

        return false;
    }

    CreatureAI* GetCreatureAI(Creature* creature) const override
    {
        if (CreatureAI* luaAI = sALE->GetAI(creature))
            return luaAI;

        return nullptr;
    }

    void OnCreatureSelectLevel(const CreatureTemplate* cinfo, Creature* creature) override
    {
        sALE->OnAllCreatureSelectLevel(cinfo, creature);
    }

    void OnBeforeCreatureSelectLevel(const CreatureTemplate* cinfo, Creature* creature, uint8& level) override
    {
        sALE->OnAllCreatureBeforeSelectLevel(cinfo, creature, level);
    }
};

class ALE_AllGameObjectScript : public AllGameObjectScript
{
public:
    ALE_AllGameObjectScript() : AllGameObjectScript("ALE_AllGameObjectScript") { }

    void OnGameObjectAddWorld(GameObject* go) override
    {
        sALE->OnAddToWorld(go);
    }

    void OnGameObjectRemoveWorld(GameObject* go) override
    {
        sALE->OnRemoveFromWorld(go);
    }

    void OnGameObjectUpdate(GameObject* go, uint32 diff) override
    {
        sALE->UpdateAI(go, diff);
    }

    bool CanGameObjectGossipHello(Player* player, GameObject* go) override
    {
        if (sALE->OnGossipHello(player, go))
            return true;

        if (sALE->OnGameObjectUse(player, go))
            return true;

        return false;
    }

    void OnGameObjectDamaged(GameObject* go, Player* player) override
    {
        sALE->OnDamaged(go, player);
    }

    void OnGameObjectDestroyed(GameObject* go, Player* player) override
    {
        sALE->OnDestroyed(go, player);
    }

    void OnGameObjectLootStateChanged(GameObject* go, uint32 state, Unit* /*unit*/) override
    {
        sALE->OnLootStateChanged(go, state);
    }

    void OnGameObjectStateChanged(GameObject* go, uint32 state) override
    {
        sALE->OnGameObjectStateChanged(go, state);
    }

    bool CanGameObjectQuestAccept(Player* player, GameObject* go, Quest const* quest) override
    {
        sALE->OnPlayerQuestAccept(player, quest);
        sALE->OnQuestAccept(player, go, quest);
        return false;
    }

    bool CanGameObjectGossipSelect(Player* player, GameObject* go, uint32 sender, uint32 action) override
    {
        if (sALE->OnGossipSelect(player, go, sender, action))
            return true;

        return false;
    }

    bool CanGameObjectGossipSelectCode(Player* player, GameObject* go, uint32 sender, uint32 action, const char* code) override
    {
        if (sALE->OnGossipSelectCode(player, go, sender, action, code))
            return true;

        return false;
    }

    bool CanGameObjectQuestReward(Player* player, GameObject* go, Quest const* quest, uint32 opt) override
    {
        if (sALE->OnQuestAccept(player, go, quest))
        {
            sALE->OnPlayerQuestAccept(player, quest);
            return false;
        }

        if (sALE->OnQuestReward(player, go, quest, opt))
            return false;

        return false;
    }

    GameObjectAI* GetGameObjectAI(GameObject* go) const override
    {
        sALE->OnSpawn(go);
        return nullptr;
    }
};

class ALE_AllItemScript : public AllItemScript
{
public:
    ALE_AllItemScript() : AllItemScript("ALE_AllItemScript") { }

    bool CanItemQuestAccept(Player* player, Item* item, Quest const* quest) override
    {
        if (sALE->OnQuestAccept(player, item, quest))
        {
            sALE->OnPlayerQuestAccept(player, quest);
            return false;
        }

        return true;
    }

    bool CanItemUse(Player* player, Item* item, SpellCastTargets const& targets) override
    {
        if (!sALE->OnUse(player, item, targets))
            return true;

        return false;
    }

    bool CanItemExpire(Player* player, ItemTemplate const* proto) override
    {
        if (sALE->OnExpire(player, proto))
            return false;

        return true;
    }

    bool CanItemRemove(Player* player, Item* item) override
    {
        if (sALE->OnRemove(player, item))
            return false;

        return true;
    }

    void OnItemGossipSelect(Player* player, Item* item, uint32 sender, uint32 action) override
    {
        sALE->HandleGossipSelectOption(player, item, sender, action, "");
    }

    void OnItemGossipSelectCode(Player* player, Item* item, uint32 sender, uint32 action, const char* code) override
    {
        sALE->HandleGossipSelectOption(player, item, sender, action, code);
    }
};

class ALE_AllMapScript : public AllMapScript
{
public:
    ALE_AllMapScript() : AllMapScript("ALE_AllMapScript", {
        ALLMAPHOOK_ON_BEFORE_CREATE_INSTANCE_SCRIPT,
        ALLMAPHOOK_ON_DESTROY_INSTANCE,
        ALLMAPHOOK_ON_CREATE_MAP,
        ALLMAPHOOK_ON_DESTROY_MAP,
        ALLMAPHOOK_ON_PLAYER_ENTER_ALL,
        ALLMAPHOOK_ON_PLAYER_LEAVE_ALL,
        ALLMAPHOOK_ON_MAP_UPDATE
    }) { }

    void OnBeforeCreateInstanceScript(InstanceMap* instanceMap, InstanceScript** instanceData, bool /*load*/, std::string /*data*/, uint32 /*completedEncounterMask*/) override
    {
        if (instanceData)
            *instanceData = sALE->GetInstanceData(instanceMap);
    }

    void OnDestroyInstance(MapInstanced* /*mapInstanced*/, Map* map) override
    {
        sALE->FreeInstanceId(map->GetInstanceId());
    }

    void OnCreateMap(Map* map) override
    {
        sALE->OnCreate(map);
    }

    void OnDestroyMap(Map* map) override
    {
        sALE->OnDestroy(map);
    }

    void OnPlayerEnterAll(Map* map, Player* player) override
    {
        sALE->OnPlayerEnter(map, player);
    }

    void OnPlayerLeaveAll(Map* map, Player* player) override
    {
        sALE->OnPlayerLeave(map, player);
    }

    void OnMapUpdate(Map* map, uint32 diff) override
    {
        sALE->OnUpdate(map, diff);
    }
};

class ALE_AuctionHouseScript : public AuctionHouseScript
{
public:
    ALE_AuctionHouseScript() : AuctionHouseScript("ALE_AuctionHouseScript", {
        AUCTIONHOUSEHOOK_ON_AUCTION_ADD,
        AUCTIONHOUSEHOOK_ON_AUCTION_REMOVE,
        AUCTIONHOUSEHOOK_ON_AUCTION_SUCCESSFUL,
        AUCTIONHOUSEHOOK_ON_AUCTION_EXPIRE
    }) { }

    void OnAuctionAdd(AuctionHouseObject* ah, AuctionEntry* entry) override
    {
        sALE->OnAdd(ah, entry);
    }

    void OnAuctionRemove(AuctionHouseObject* ah, AuctionEntry* entry) override
    {
        sALE->OnRemove(ah, entry);
    }

    void OnAuctionSuccessful(AuctionHouseObject* ah, AuctionEntry* entry) override
    {
        sALE->OnSuccessful(ah, entry);
    }

    void OnAuctionExpire(AuctionHouseObject* ah, AuctionEntry* entry) override
    {
        sALE->OnExpire(ah, entry);
    }
};

class ALE_BGScript : public BGScript
{
public:
    ALE_BGScript() : BGScript("ALE_BGScript", {
        ALLBATTLEGROUNDHOOK_ON_BATTLEGROUND_START,
        ALLBATTLEGROUNDHOOK_ON_BATTLEGROUND_END,
        ALLBATTLEGROUNDHOOK_ON_BATTLEGROUND_DESTROY,
        ALLBATTLEGROUNDHOOK_ON_BATTLEGROUND_CREATE
    }) { }

    void OnBattlegroundStart(Battleground* bg) override
    {
        sALE->OnBGStart(bg, bg->GetBgTypeID(), bg->GetInstanceID());
    }

    void OnBattlegroundEnd(Battleground* bg, TeamId winnerTeam) override
    {
        sALE->OnBGEnd(bg, bg->GetBgTypeID(), bg->GetInstanceID(), winnerTeam);
    }

    void OnBattlegroundDestroy(Battleground* bg) override
    {
        sALE->OnBGDestroy(bg, bg->GetBgTypeID(), bg->GetInstanceID());
    }

    void OnBattlegroundCreate(Battleground* bg) override
    {
        sALE->OnBGCreate(bg, bg->GetBgTypeID(), bg->GetInstanceID());
    }
};

class ALE_CommandSC : public CommandSC
{
public:
    ALE_CommandSC() : CommandSC("ALE_CommandSC", {
        ALLCOMMANDHOOK_ON_TRY_EXECUTE_COMMAND
    }) { }

    bool OnTryExecuteCommand(ChatHandler& handler, std::string_view cmdStr) override
    {
        if (!sALE->OnCommand(handler, std::string(cmdStr).c_str()))
        {
            return false;
        }

        return true;
    }
};

class ALE_ALEScript : public ALEScript
{
public:
    ALE_ALEScript() : ALEScript("ALE_ALEScript") { }

    // Weather
    void OnWeatherChange(Weather* weather, WeatherState state, float grade) override
    {
        sALE->OnChange(weather, weather->GetZone(), state, grade);
    }

    // AreaTriger
    bool CanAreaTrigger(Player* player, AreaTrigger const* trigger) override
    {
        if (sALE->OnAreaTrigger(player, trigger))
            return true;

        return false;
    }
};

class ALE_GameEventScript : public GameEventScript
{
public:
    ALE_GameEventScript() : GameEventScript("ALE_GameEventScript", {
        GAMEEVENTHOOK_ON_START,
        GAMEEVENTHOOK_ON_STOP
    }) { }

    void OnStart(uint16 eventID) override
    {
        sALE->OnGameEventStart(eventID);
    }

    void OnStop(uint16 eventID) override
    {
        sALE->OnGameEventStop(eventID);
    }
};

class ALE_GroupScript : public GroupScript
{
public:
    ALE_GroupScript() : GroupScript("ALE_GroupScript", {
        GROUPHOOK_ON_ADD_MEMBER,
        GROUPHOOK_ON_INVITE_MEMBER,
        GROUPHOOK_ON_REMOVE_MEMBER,
        GROUPHOOK_ON_CHANGE_LEADER,
        GROUPHOOK_ON_DISBAND,
        GROUPHOOK_ON_CREATE
    }) { }

    void OnAddMember(Group* group, ObjectGuid guid) override
    {
        sALE->OnAddMember(group, guid);
    }

    void OnInviteMember(Group* group, ObjectGuid guid) override
    {
        sALE->OnInviteMember(group, guid);
    }

    void OnRemoveMember(Group* group, ObjectGuid guid, RemoveMethod method, ObjectGuid /* kicker */, const char* /* reason */) override
    {
        sALE->OnRemoveMember(group, guid, method);
    }

    void OnChangeLeader(Group* group, ObjectGuid newLeaderGuid, ObjectGuid oldLeaderGuid) override
    {
        sALE->OnChangeLeader(group, newLeaderGuid, oldLeaderGuid);
    }

    void OnDisband(Group* group) override
    {
        sALE->OnDisband(group);
    }

    void OnCreate(Group* group, Player* leader) override
    {
        sALE->OnCreate(group, leader->GetGUID(), group->GetGroupType());
    }
};

class ALE_GuildScript : public GuildScript
{
public:
    ALE_GuildScript() : GuildScript("ALE_GuildScript", {
        GUILDHOOK_ON_ADD_MEMBER,
        GUILDHOOK_ON_REMOVE_MEMBER,
        GUILDHOOK_ON_MOTD_CHANGED,
        GUILDHOOK_ON_INFO_CHANGED,
        GUILDHOOK_ON_CREATE,
        GUILDHOOK_ON_DISBAND,
        GUILDHOOK_ON_MEMBER_WITDRAW_MONEY,
        GUILDHOOK_ON_MEMBER_DEPOSIT_MONEY,
        GUILDHOOK_ON_ITEM_MOVE,
        GUILDHOOK_ON_EVENT,
        GUILDHOOK_ON_BANK_EVENT
    }) { }

    void OnAddMember(Guild* guild, Player* player, uint8& plRank) override
    {
        sALE->OnAddMember(guild, player, plRank);
    }

    void OnRemoveMember(Guild* guild, Player* player, bool isDisbanding, bool /*isKicked*/) override
    {
        sALE->OnRemoveMember(guild, player, isDisbanding);
    }

    void OnMOTDChanged(Guild* guild, const std::string& newMotd) override
    {
        sALE->OnMOTDChanged(guild, newMotd);
    }

    void OnInfoChanged(Guild* guild, const std::string& newInfo) override
    {
        sALE->OnInfoChanged(guild, newInfo);
    }

    void OnCreate(Guild* guild, Player* leader, const std::string& name) override
    {
        sALE->OnCreate(guild, leader, name);
    }

    void OnDisband(Guild* guild) override
    {
        sALE->OnDisband(guild);
    }

    void OnMemberWitdrawMoney(Guild* guild, Player* player, uint32& amount, bool isRepair) override
    {
        sALE->OnMemberWitdrawMoney(guild, player, amount, isRepair);
    }

    void OnMemberDepositMoney(Guild* guild, Player* player, uint32& amount) override
    {
        sALE->OnMemberDepositMoney(guild, player, amount);
    }

    void OnItemMove(Guild* guild, Player* player, Item* pItem, bool isSrcBank, uint8 srcContainer, uint8 srcSlotId,
        bool isDestBank, uint8 destContainer, uint8 destSlotId) override
    {
        sALE->OnItemMove(guild, player, pItem, isSrcBank, srcContainer, srcSlotId, isDestBank, destContainer, destSlotId);
    }

    void OnEvent(Guild* guild, uint8 eventType, ObjectGuid::LowType playerGuid1, ObjectGuid::LowType playerGuid2, uint8 newRank) override
    {
        sALE->OnEvent(guild, eventType, playerGuid1, playerGuid2, newRank);
    }

    void OnBankEvent(Guild* guild, uint8 eventType, uint8 tabId, ObjectGuid::LowType playerGuid, uint32 itemOrMoney, uint16 itemStackCount, uint8 destTabId) override
    {
        sALE->OnBankEvent(guild, eventType, tabId, playerGuid, itemOrMoney, itemStackCount, destTabId);
    }
};

class ALE_LootScript : public LootScript
{
public:
    ALE_LootScript() : LootScript("ALE_LootScript", {
        LOOTHOOK_ON_LOOT_MONEY
    }) { }

    void OnLootMoney(Player* player, uint32 gold) override
    {
        sALE->OnLootMoney(player, gold);
    }
};

class ALE_MiscScript : public MiscScript
{
public:
    ALE_MiscScript() : MiscScript("ALE_MiscScript", {
        MISCHOOK_GET_DIALOG_STATUS
    }) { }

    void GetDialogStatus(Player* player, Object* questgiver) override
    {
        if (questgiver->GetTypeId() == TYPEID_GAMEOBJECT)
            sALE->GetDialogStatus(player, questgiver->ToGameObject());
        else if (questgiver->GetTypeId() == TYPEID_UNIT)
            sALE->GetDialogStatus(player, questgiver->ToCreature());
    }
};

class ALE_PetScript : public PetScript
{
public:
    ALE_PetScript() : PetScript("ALE_PetScript") { }

    AC_USES_HOOK(PetScript, OnPetAddToWorld);
    void OnPetAddToWorld(Pet* pet) override
    {
        sALE->OnPetAddedToWorld(pet->GetOwner(), pet);
    }
};

class ALE_PlayerScript : public PlayerScript
{
public:
    ALE_PlayerScript() : PlayerScript("ALE_PlayerScript") { }

    AC_USES_HOOK(PlayerScript, OnPlayerResurrect);
    void OnPlayerResurrect(Player* player, float /*restore_percent*/, bool& /*applySickness*/) override
    {
        sALE->OnResurrect(player);
    }

    AC_USES_HOOK(PlayerScript, OnPlayerCanUseChat);
    bool OnPlayerCanUseChat(Player* player, uint32 type, uint32 lang, std::string& msg) override
    {
        if (type != CHAT_MSG_SAY && type != CHAT_MSG_YELL && type != CHAT_MSG_EMOTE)
            return true;

        if (!sALE->OnChat(player, type, lang, msg))
            return false;

        return true;
    }

    AC_USES_HOOK(PlayerScript, OnPlayerCanUsePrivateChat);
    bool OnPlayerCanUseChat(Player* player, uint32 type, uint32 lang, std::string& msg, Player* target) override
    {
        if (!sALE->OnChat(player, type, lang, msg, target))
            return false;

        return true;
    }

    AC_USES_HOOK(PlayerScript, OnPlayerCanUseGroupChat);
    bool OnPlayerCanUseChat(Player* player, uint32 type, uint32 lang, std::string& msg, Group* group) override
    {
        if (!sALE->OnChat(player, type, lang, msg, group))
            return false;

        return true;
    }

    AC_USES_HOOK(PlayerScript, OnPlayerCanUseGuildChat);
    bool OnPlayerCanUseChat(Player* player, uint32 type, uint32 lang, std::string& msg, Guild* guild) override
    {
        if (!sALE->OnChat(player, type, lang, msg, guild))
            return false;

        return true;
    }

    AC_USES_HOOK(PlayerScript, OnPlayerCanUseChannelChat);
    bool OnPlayerCanUseChat(Player* player, uint32 type, uint32 lang, std::string& msg, Channel* channel) override
    {
        if (!sALE->OnChat(player, type, lang, msg, channel))
            return false;

        return true;
    }

    AC_USES_HOOK(PlayerScript, OnPlayerLootItem);
    void OnPlayerLootItem(Player* player, Item* item, uint32 count, ObjectGuid lootguid) override
    {
        sALE->OnLootItem(player, item, count, lootguid);
    }

    AC_USES_HOOK(PlayerScript, OnPlayerLearnTalents);
    void OnPlayerLearnTalents(Player* player, uint32 talentId, uint32 talentRank, uint32 spellid) override
    {
        sALE->OnLearnTalents(player, talentId, talentRank, spellid);
    }

    AC_USES_HOOK(PlayerScript, OnPlayerCanUseItem);
    bool OnPlayerCanUseItem(Player* player, ItemTemplate const* proto, InventoryResult& result) override
    {
        result = sALE->OnCanUseItem(player, proto->ItemId);
        return result != EQUIP_ERR_OK ? false : true;
    }

    AC_USES_HOOK(PlayerScript, OnPlayerEquip);
    void OnPlayerEquip(Player* player, Item* it, uint8 bag, uint8 slot, bool /*update*/) override
    {
        sALE->OnEquip(player, it, bag, slot);
    }

    AC_USES_HOOK(PlayerScript, OnPlayerEnterCombat);
    void OnPlayerEnterCombat(Player* player, Unit* enemy) override
    {
        sALE->OnPlayerEnterCombat(player, enemy);
    }

    AC_USES_HOOK(PlayerScript, OnPlayerLeaveCombat);
    void OnPlayerLeaveCombat(Player* player) override
    {
        sALE->OnPlayerLeaveCombat(player);
    }

    AC_USES_HOOK(PlayerScript, OnPlayerCanRepopAtGraveyard);
    bool OnPlayerCanRepopAtGraveyard(Player* player) override
    {
        sALE->OnRepop(player);
        return true;
    }

    AC_USES_HOOK(PlayerScript, OnPlayerQuestAbandon);
    void OnPlayerQuestAbandon(Player* player, uint32 questId) override
    {
        sALE->OnQuestAbandon(player, questId);
    }

    AC_USES_HOOK(PlayerScript, OnPlayerMapChanged);
    void OnPlayerMapChanged(Player* player) override
    {
        sALE->OnMapChanged(player);
    }

    AC_USES_HOOK(PlayerScript, OnPlayerGossipSelect);
    void OnPlayerGossipSelect(Player* player, uint32 menu_id, uint32 sender, uint32 action) override
    {
        sALE->HandleGossipSelectOption(player, menu_id, sender, action, "");
    }

    AC_USES_HOOK(PlayerScript, OnPlayerGossipSelectCode);
    void OnPlayerGossipSelectCode(Player* player, uint32 menu_id, uint32 sender, uint32 action, const char* code) override
    {
        sALE->HandleGossipSelectOption(player, menu_id, sender, action, code);
    }

    AC_USES_HOOK(PlayerScript, OnPlayerPVPKill);
    void OnPlayerPVPKill(Player* killer, Player* killed) override
    {
        sALE->OnPVPKill(killer, killed);
    }

    AC_USES_HOOK(PlayerScript, OnPlayerCreatureKill);
    void OnPlayerCreatureKill(Player* killer, Creature* killed) override
    {
        sALE->OnCreatureKill(killer, killed);
    }

    AC_USES_HOOK(PlayerScript, OnPlayerKilledByCreature);
    void OnPlayerKilledByCreature(Creature* killer, Player* killed) override
    {
        sALE->OnPlayerKilledByCreature(killer, killed);
    }

    AC_USES_HOOK(PlayerScript, OnPlayerLevelChanged);
    void OnPlayerLevelChanged(Player* player, uint8 oldLevel) override
    {
        sALE->OnLevelChanged(player, oldLevel);
    }

    AC_USES_HOOK(PlayerScript, OnPlayerFreeTalentPointsChanged);
    void OnPlayerFreeTalentPointsChanged(Player* player, uint32 points) override
    {
        sALE->OnFreeTalentPointsChanged(player, points);
    }

    AC_USES_HOOK(PlayerScript, OnPlayerTalentsReset);
    void OnPlayerTalentsReset(Player* player, bool noCost) override
    {
        sALE->OnTalentsReset(player, noCost);
    }

    AC_USES_HOOK(PlayerScript, OnPlayerMoneyChanged);
    void OnPlayerMoneyChanged(Player* player, int32& amount) override
    {
        sALE->OnMoneyChanged(player, amount);
    }

    AC_USES_HOOK(PlayerScript, OnPlayerGiveXP);
    void OnPlayerGiveXP(Player* player, uint32& amount, Unit* victim, uint8 xpSource) override
    {
        sALE->OnGiveXP(player, amount, victim, xpSource);
    }

    AC_USES_HOOK(PlayerScript, OnPlayerReputationChange);
    bool OnPlayerReputationChange(Player* player, uint32 factionID, int32& standing, bool incremental) override
    {
        return sALE->OnReputationChange(player, factionID, standing, incremental);
    }

    AC_USES_HOOK(PlayerScript, OnPlayerDuelRequest);
    void OnPlayerDuelRequest(Player* target, Player* challenger) override
    {
        sALE->OnDuelRequest(target, challenger);
    }

    AC_USES_HOOK(PlayerScript, OnPlayerDuelStart);
    void OnPlayerDuelStart(Player* player1, Player* player2) override
    {
        sALE->OnDuelStart(player1, player2);
    }

    AC_USES_HOOK(PlayerScript, OnPlayerDuelEnd);
    void OnPlayerDuelEnd(Player* winner, Player* loser, DuelCompleteType type) override
    {
        sALE->OnDuelEnd(winner, loser, type);
    }

    AC_USES_HOOK(PlayerScript, OnPlayerEmote);
    void OnPlayerEmote(Player* player, uint32 emote) override
    {
        sALE->OnEmote(player, emote);
    }

    AC_USES_HOOK(PlayerScript, OnPlayerTextEmote);
    void OnPlayerTextEmote(Player* player, uint32 textEmote, uint32 emoteNum, ObjectGuid guid) override
    {
        sALE->OnTextEmote(player, textEmote, emoteNum, guid);
    }

    AC_USES_HOOK(PlayerScript, OnPlayerSpellCast);
    void OnPlayerSpellCast(Player* player, Spell* spell, bool skipCheck) override
    {
        sALE->OnPlayerSpellCast(player, spell, skipCheck);
    }

    AC_USES_HOOK(PlayerScript, OnPlayerLogin);
    void OnPlayerLogin(Player* player) override
    {
        sALE->OnLogin(player);
    }

    AC_USES_HOOK(PlayerScript, OnPlayerLogout);
    void OnPlayerLogout(Player* player) override
    {
        sALE->OnLogout(player);
    }

    AC_USES_HOOK(PlayerScript, OnPlayerCreate);
    void OnPlayerCreate(Player* player) override
    {
        sALE->OnCreate(player);
    }

    AC_USES_HOOK(PlayerScript, OnPlayerSave);
    void OnPlayerSave(Player* player) override
    {
        sALE->OnSave(player);
    }

    AC_USES_HOOK(PlayerScript, OnPlayerDelete);
    void OnPlayerDelete(ObjectGuid guid, uint32 /*accountId*/) override
    {
        sALE->OnDelete(guid.GetCounter());
    }

    AC_USES_HOOK(PlayerScript, OnPlayerBindToInstance);
    void OnPlayerBindToInstance(Player* player, Difficulty difficulty, uint32 mapid, bool permanent) override
    {
        sALE->OnBindToInstance(player, difficulty, mapid, permanent);
    }

    AC_USES_HOOK(PlayerScript, OnPlayerUpdateArea);
    void OnPlayerUpdateArea(Player* player, uint32 oldArea, uint32 newArea) override
    {
        sALE->OnUpdateArea(player, oldArea, newArea);
    }

    AC_USES_HOOK(PlayerScript, OnPlayerUpdateZone);
    void OnPlayerUpdateZone(Player* player, uint32 newZone, uint32 newArea) override
    {
        sALE->OnUpdateZone(player, newZone, newArea);
    }

    AC_USES_HOOK(PlayerScript, OnPlayerFirstLogin);
    void OnPlayerFirstLogin(Player* player) override
    {
        sALE->OnFirstLogin(player);
    }

    AC_USES_HOOK(PlayerScript, OnPlayerLearnSpell);
    void OnPlayerLearnSpell(Player* player, uint32 spellId) override
    {
        sALE->OnLearnSpell(player, spellId);
    }

    AC_USES_HOOK(PlayerScript, OnPlayerAchievementComplete);
    void OnPlayerAchievementComplete(Player* player, AchievementEntry const* achievement) override
    {
        sALE->OnAchiComplete(player, achievement);
    }

    AC_USES_HOOK(PlayerScript, OnPlayerFfaPvpStateUpdate);
    void OnPlayerFfaPvpStateUpdate(Player* player, bool IsFlaggedForFfaPvp) override
    {
        sALE->OnFfaPvpStateUpdate(player, IsFlaggedForFfaPvp);
    }

    AC_USES_HOOK(PlayerScript, OnPlayerCanInitTrade);
    bool OnPlayerCanInitTrade(Player* player, Player* target) override
    {
        return sALE->OnCanInitTrade(player, target);
    }

    AC_USES_HOOK(PlayerScript, OnPlayerCanSendMail);
    bool OnPlayerCanSendMail(Player* player, ObjectGuid receiverGuid, ObjectGuid mailbox, std::string& subject, std::string& body, uint32 money, uint32 cod, Item* item) override
    {
        return sALE->OnCanSendMail(player, receiverGuid, mailbox, subject, body, money, cod, item);
    }

    AC_USES_HOOK(PlayerScript, OnPlayerCanJoinLfg);
    bool OnPlayerCanJoinLfg(Player* player, uint8 roles, lfg::LfgDungeonSet& dungeons, const std::string& comment) override
    {
        return sALE->OnCanJoinLfg(player, roles, dungeons, comment);
    }

    AC_USES_HOOK(PlayerScript, OnPlayerQuestRewardItem);
    void OnPlayerQuestRewardItem(Player* player, Item* item, uint32 count) override
    {
        sALE->OnQuestRewardItem(player, item, count);
    }

    AC_USES_HOOK(PlayerScript, OnPlayerGroupRollRewardItem);
    void OnPlayerGroupRollRewardItem(Player* player, Item* item, uint32 count, RollVote voteType, Roll* roll) override
    {
        sALE->OnGroupRollRewardItem(player, item, count, voteType, roll);
    }

    AC_USES_HOOK(PlayerScript, OnPlayerCreateItem);
    void OnPlayerCreateItem(Player* player, Item* item, uint32 count) override
    {
        sALE->OnCreateItem(player, item, count);
    }

    AC_USES_HOOK(PlayerScript, OnPlayerStoreNewItem);
    void OnPlayerStoreNewItem(Player* player, Item* item, uint32 count) override
    {
        sALE->OnStoreNewItem(player, item, count);
    }

    AC_USES_HOOK(PlayerScript, OnPlayerCompleteQuest);
    void OnPlayerCompleteQuest(Player* player, Quest const* quest) override
    {
        sALE->OnPlayerCompleteQuest(player, quest);
    }

    AC_USES_HOOK(PlayerScript, OnPlayerCanGroupInvite);
    bool OnPlayerCanGroupInvite(Player* player, std::string& memberName) override
    {
        return sALE->OnCanGroupInvite(player, memberName);
    }

    AC_USES_HOOK(PlayerScript, OnPlayerBattlegroundDesertion);
    void OnPlayerBattlegroundDesertion(Player* player, const BattlegroundDesertionType type) override
    {
        sALE->OnBattlegroundDesertion(player, type);
    }

    AC_USES_HOOK(PlayerScript, OnPlayerCreatureKilledByPet);
    void OnPlayerCreatureKilledByPet(Player* player, Creature* killed) override
    {
        sALE->OnCreatureKilledByPet(player, killed);
    }

    AC_USES_HOOK(PlayerScript, OnPlayerCanUpdateSkill);
    bool OnPlayerCanUpdateSkill(Player* player, uint32 skill_id) override
    {
        return sALE->OnPlayerCanUpdateSkill(player, skill_id);
    }

    AC_USES_HOOK(PlayerScript, OnPlayerBeforeUpdateSkill);
    void OnPlayerBeforeUpdateSkill(Player* player, uint32 skill_id, uint32& value, uint32 max, uint32 step) override
    {
        sALE->OnPlayerBeforeUpdateSkill(player, skill_id, value, max, step);
    }

    AC_USES_HOOK(PlayerScript, OnPlayerUpdateSkill);
    void OnPlayerUpdateSkill(Player* player, uint32 skill_id, uint32 value, uint32 max, uint32 step, uint32 new_value) override
    {
        sALE->OnPlayerUpdateSkill(player, skill_id, value, max, step, new_value);
    }

    AC_USES_HOOK(PlayerScript, OnPlayerCanResurrect);
    bool OnPlayerCanResurrect(Player* player) override
    {
        return sALE->CanPlayerResurrect(player);
    }

    AC_USES_HOOK(PlayerScript, OnPlayerReleasedGhost);
    void OnPlayerReleasedGhost(Player* player) override
    {
        sALE->OnPlayerReleasedGhost(player);
    }
};

class ALE_ServerScript : public ServerScript
{
public:
    ALE_ServerScript() : ServerScript("ALE_ServerScript", {
        SERVERHOOK_CAN_PACKET_SEND,
        SERVERHOOK_CAN_PACKET_RECEIVE
    }) { }

    bool CanPacketSend(WorldSession* session, WorldPacket const& packet) override
    {
        if (!sALE->OnPacketSend(session, packet))
            return false;

        return true;
    }

    bool CanPacketReceive(WorldSession* session, WorldPacket const& packet) override
    {
        if (!sALE->OnPacketReceive(session, packet))
            return false;

        return true;
    }
};

class ALE_SpellSC : public SpellSC
{
public:
    ALE_SpellSC() : SpellSC("ALE_SpellSC", {
        ALLSPELLHOOK_ON_DUMMY_EFFECT_GAMEOBJECT,
        ALLSPELLHOOK_ON_DUMMY_EFFECT_CREATURE,
        ALLSPELLHOOK_ON_DUMMY_EFFECT_ITEM,
        ALLSPELLHOOK_ON_CAST_CANCEL,
        ALLSPELLHOOK_ON_CAST,
        ALLSPELLHOOK_ON_PREPARE
    }) { }

    void OnDummyEffect(WorldObject* caster, uint32 spellID, SpellEffIndex effIndex, GameObject* gameObjTarget) override
    {
        sALE->OnDummyEffect(caster, spellID, effIndex, gameObjTarget);
    }

    void OnDummyEffect(WorldObject* caster, uint32 spellID, SpellEffIndex effIndex, Creature* creatureTarget) override
    {
        sALE->OnDummyEffect(caster, spellID, effIndex, creatureTarget);
    }

    void OnDummyEffect(WorldObject* caster, uint32 spellID, SpellEffIndex effIndex, Item* itemTarget) override
    {
        sALE->OnDummyEffect(caster, spellID, effIndex, itemTarget);
    }

    void OnSpellCastCancel(Spell* spell, Unit* caster, SpellInfo const* spellInfo, bool bySelf) override
    {
        sALE->OnSpellCastCancel(caster, spell, spellInfo, bySelf);
    }

    void OnSpellCast(Spell* spell, Unit* caster, SpellInfo const* spellInfo, bool skipCheck) override
    {
        sALE->OnSpellCast(caster, spell, spellInfo, skipCheck);
    }

    void OnSpellPrepare(Spell* spell, Unit* caster, SpellInfo const* spellInfo) override
    {
        sALE->OnSpellPrepare(caster, spell, spellInfo);
    }
};

class ALE_VehicleScript : public VehicleScript
{
public:
    ALE_VehicleScript() : VehicleScript("ALE_VehicleScript") { }

    void OnInstall(Vehicle* veh) override
    {
        sALE->OnInstall(veh);
    }

    void OnUninstall(Vehicle* veh) override
    {
        sALE->OnUninstall(veh);
    }

    void OnInstallAccessory(Vehicle* veh, Creature* accessory) override
    {
        sALE->OnInstallAccessory(veh, accessory);
    }

    void OnAddPassenger(Vehicle* veh, Unit* passenger, int8 seatId) override
    {
        sALE->OnAddPassenger(veh, passenger, seatId);
    }

    void OnRemovePassenger(Vehicle* veh, Unit* passenger) override
    {
        sALE->OnRemovePassenger(veh, passenger);
    }
};

class ALE_WorldObjectScript : public WorldObjectScript
{
public:
    ALE_WorldObjectScript() : WorldObjectScript("ALE_WorldObjectScript", {
        WORLDOBJECTHOOK_ON_WORLD_OBJECT_DESTROY,
        WORLDOBJECTHOOK_ON_WORLD_OBJECT_CREATE,
        WORLDOBJECTHOOK_ON_WORLD_OBJECT_SET_MAP,
        WORLDOBJECTHOOK_ON_WORLD_OBJECT_UPDATE
    }) { }

    void OnWorldObjectDestroy(WorldObject* object) override
    {
        delete object->ALEEvents;
        object->ALEEvents = nullptr;
    }

    void OnWorldObjectCreate(WorldObject* object) override
    {
        object->ALEEvents = nullptr;
    }

    void OnWorldObjectSetMap(WorldObject* object, Map* /*map*/) override
    {
        if (!object->ALEEvents)
            object->ALEEvents = new ALEEventProcessor(&ALE::GALE, object);
    }

    void OnWorldObjectUpdate(WorldObject* object, uint32 diff) override
    {
        object->ALEEvents->Update(diff);
    }
};

class ALE_WorldScript : public WorldScript
{
public:
    ALE_WorldScript() : WorldScript("ALE_WorldScript") { }

    AC_USES_HOOK(WorldScript, OnOpenStateChange);
    void OnOpenStateChange(bool open) override
    {
        sALE->OnOpenStateChange(open);
    }

    AC_USES_HOOK(WorldScript, OnBeforeConfigLoad);
    void OnBeforeConfigLoad(bool reload) override
    {
        ALEConfig::GetInstance().Initialize(reload);
        if (!reload)
        {
            ///- Initialize Lua Engine
            LOG_INFO("ALE", "Initialize ALE Lua Engine...");
            ALE::Initialize();
        }

        sALE->OnConfigLoad(reload, true);
    }

    AC_USES_HOOK(WorldScript, OnAfterConfigLoad);
    void OnAfterConfigLoad(bool reload) override
    {
        sALE->OnConfigLoad(reload, false);
    }

    AC_USES_HOOK(WorldScript, OnShutdownInitiate);
    void OnShutdownInitiate(ShutdownExitCode code, ShutdownMask mask) override
    {
        sALE->OnShutdownInitiate(code, mask);
    }

    AC_USES_HOOK(WorldScript, OnShutdownCancel);
    void OnShutdownCancel() override
    {
        sALE->OnShutdownCancel();
    }

    AC_USES_HOOK(WorldScript, OnUpdate);
    void OnUpdate(uint32 diff) override
    {
        sALE->OnWorldUpdate(diff);
    }

    AC_USES_HOOK(WorldScript, OnStartup);
    void OnStartup() override
    {
        sALE->OnStartup();
    }

    AC_USES_HOOK(WorldScript, OnShutdown);
    void OnShutdown() override
    {
        sALE->OnShutdown();
    }

    AC_USES_HOOK(WorldScript, OnAfterUnloadAllMaps);
    void OnAfterUnloadAllMaps() override
    {
        ALE::Uninitialize();
    }

    AC_USES_HOOK(WorldScript, OnBeforeWorldInitialized);
    void OnBeforeWorldInitialized() override
    {
        ///- Run ALE scripts.
        // in multithread foreach: run scripts
        sALE->RunScripts();
        sALE->OnConfigLoad(false, false); // Must be done after ALE is initialized and scripts have run.
    }
};

class ALE_TicketScript : public TicketScript
{
public:
    ALE_TicketScript() : TicketScript("ALE_TicketScript", {
        TICKETHOOK_ON_TICKET_CREATE,
        TICKETHOOK_ON_TICKET_UPDATE_LAST_CHANGE,
        TICKETHOOK_ON_TICKET_CLOSE,
        TICKETHOOK_ON_TICKET_RESOLVE
    }) { }

    void OnTicketCreate(GmTicket* ticket) override
    {
        sALE->OnTicketCreate(ticket);
    }

    void OnTicketUpdateLastChange(GmTicket* ticket) override
    {
        sALE->OnTicketUpdateLastChange(ticket);
    }

    void OnTicketClose(GmTicket* ticket) override
    {
        sALE->OnTicketClose(ticket);
    }

    void OnTicketResolve(GmTicket* ticket) override
    {
        sALE->OnTicketResolve(ticket);
    }
};

class ALE_UnitScript : public UnitScript
{
public:
    ALE_UnitScript() : UnitScript("ALE_UnitScript") { }

    void OnAuraApply(Unit* unit, Aura* aura) override
    {
        if (!unit || !aura) return;

        if (unit->IsPlayer())
            sALE->OnPlayerAuraApply(unit->ToPlayer(), aura);

        if (unit->IsCreature())
        {
            sALE->OnCreatureAuraApply(unit->ToCreature(), aura);
            sALE->OnAllCreatureAuraApply(unit->ToCreature(), aura);
        }
    }

    void OnAuraRemove(Unit* unit, AuraApplication* aurApp, AuraRemoveMode mode) override
    {
        if (!unit || !aurApp->GetBase()) return;

        if (unit->IsPlayer())
            sALE->OnPlayerAuraRemove(unit->ToPlayer(), aurApp->GetBase(), mode);

        if (unit->IsCreature())
        {
            sALE->OnCreatureAuraRemove(unit->ToCreature(), aurApp->GetBase(), mode);
            sALE->OnAllCreatureAuraRemove(unit->ToCreature(), aurApp->GetBase(), mode);
        }
    }

    void OnHeal(Unit* healer, Unit* receiver, uint32& gain) override
    {
        if (!receiver || !healer) return;

        if (healer->IsPlayer())
            sALE->OnPlayerHeal(healer->ToPlayer(), receiver, gain);

        if (healer->IsCreature())
        {
            sALE->OnCreatureHeal(healer->ToCreature(), receiver, gain);
            sALE->OnAllCreatureHeal(healer->ToCreature(), receiver, gain);
        }
    }

    void OnDamage(Unit* attacker, Unit* receiver, uint32& damage) override
    {
        if (!attacker || !receiver) return;

        if (attacker->IsPlayer())
            sALE->OnPlayerDamage(attacker->ToPlayer(), receiver, damage);

        if (attacker->IsCreature())
        {
            sALE->OnCreatureDamage(attacker->ToCreature(), receiver, damage);
            sALE->OnAllCreatureDamage(attacker->ToCreature(), receiver, damage);
        }
    }

    void ModifyPeriodicDamageAurasTick(Unit* target, Unit* attacker, uint32& damage, SpellInfo const* spellInfo) override
    {
        if (!target || !attacker) return;

        if (attacker->IsPlayer())
            sALE->OnPlayerModifyPeriodicDamageAurasTick(attacker->ToPlayer(), target, damage, spellInfo);

        if (attacker->IsCreature())
        {
            sALE->OnCreatureModifyPeriodicDamageAurasTick(attacker->ToCreature(), target, damage, spellInfo);
            sALE->OnAllCreatureModifyPeriodicDamageAurasTick(attacker->ToCreature(), target, damage, spellInfo);
        }
    }

    void ModifyMeleeDamage(Unit* target, Unit* attacker, uint32& damage) override
    {
        if (!target || !attacker) return;

        if (attacker->IsPlayer())
            sALE->OnPlayerModifyMeleeDamage(attacker->ToPlayer(), target, damage);

        if (attacker->IsCreature())
        {
            sALE->OnCreatureModifyMeleeDamage(attacker->ToCreature(), target, damage);
            sALE->OnAllCreatureModifyMeleeDamage(attacker->ToCreature(), target, damage);
        }
    }

    void ModifySpellDamageTaken(Unit* target, Unit* attacker, int32& damage, SpellInfo const* spellInfo) override
    {
        if (!target || !attacker) return;

        if (attacker->IsPlayer())
            sALE->OnPlayerModifySpellDamageTaken(attacker->ToPlayer(), target, damage, spellInfo);

        if (attacker->IsCreature())
        {
            sALE->OnCreatureModifySpellDamageTaken(attacker->ToCreature(), target, damage, spellInfo);
            sALE->OnAllCreatureModifySpellDamageTaken(attacker->ToCreature(), target, damage, spellInfo);
        }
    }

    void ModifyHealReceived(Unit* target, Unit* healer, uint32& heal, SpellInfo const* spellInfo) override
    {
        if (!target || !healer) return;

        if (healer->IsPlayer())
            sALE->OnPlayerModifyHealReceived(healer->ToPlayer(), target, heal, spellInfo);

        if (healer->IsCreature())
        {
            sALE->OnCreatureModifyHealReceived(healer->ToCreature(), target, heal, spellInfo);
            sALE->OnAllCreatureModifyHealReceived(healer->ToCreature(), target, heal, spellInfo);
        }
    }

    uint32 DealDamage(Unit* AttackerUnit, Unit* pVictim, uint32 damage, DamageEffectType damagetype) override
    {
        if (!AttackerUnit || !pVictim) return damage;

        if (AttackerUnit->IsPlayer())
            return sALE->OnPlayerDealDamage(AttackerUnit->ToPlayer(), pVictim, damage, damagetype);

        if (AttackerUnit->IsCreature())
            return sALE->OnCreatureDealDamage(AttackerUnit->ToCreature(), pVictim, damage, damagetype);

        return damage;
    }
};

// Group all custom scripts
void AddSC_ALE()
{
    new ALE_AllCreatureScript();
    new ALE_AllGameObjectScript();
    new ALE_AllItemScript();
    new ALE_AllMapScript();
    new ALE_AuctionHouseScript();
    new ALE_BGScript();
    new ALE_CommandSC();
    new ALE_ALEScript();
    new ALE_GameEventScript();
    new ALE_GroupScript();
    new ALE_GuildScript();
    new ALE_LootScript();
    new ALE_MiscScript();
    new ALE_PetScript();
    new ALE_PlayerScript();
    new ALE_ServerScript();
    new ALE_SpellSC();
    new ALE_TicketScript();
    new ALE_VehicleScript();
    new ALE_WorldObjectScript();
    new ALE_WorldScript();
    new ALE_UnitScript();
}
