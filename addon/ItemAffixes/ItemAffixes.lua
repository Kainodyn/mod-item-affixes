-- ItemAffixes.lua (Conquest of Azeroth)
-- Shows the server-rolled affixes of items in tooltips: own bags and equipment, inspect,
-- auction house, trade window, chat links and comparison tooltips. Affixes are rolled
-- automatically by the server; this addon only displays them and drives the Reforge window.
--
-- Protocol: addon messages with prefix AFXM, whispered to the player themself.
--   client -> server: CONFIG, ALLDATA, DATA|bag|slot, PEEK|uid, PEEKUNIT|name|slot,
--                     PEEKUNITALL|name, PEEKAUCTION|owner|itemId|buyout|type|index|offset,
--                     TRADEPEEK|slot, MYTRADEPEEK|slot, REFORGE_*
--   server -> client: DATA, PEEKDATA, INSPECTDATA, AUCTIONDATA, TRADEGUID, MYTRADEGUID,
--                     CONFIG, ERR, D3ROLL, REFORGE_OPEN, REFORGESTATUS, REFORGEOPTS,
--                     REFORGEPREVIEW, REFORGEPREVIEWDATA
-- Slot entries are "s<idx>:<state>:<text>", state A = applied, U = nothing eligible.
-- A text starting with "!" is a crit roll (shown in gold).

local ADDON_NAME = "ItemAffixes"
local PREFIX = "AFXM"
local AFXM = {}
_G["AFXM"] = AFXM

AFX_DEBUG = false

if RegisterAddonMessagePrefix then
    RegisterAddonMessagePrefix(PREFIX)
end

local COLOR_AFFIX = "|cff44DDFF"
local COLOR_CRIT  = "|cffFFD700"
local COLOR_WAIT  = "|cff888888"
local REQUEST_COOLDOWN = 2.0

-- Own items, keyed by position: cache[bag][slot] (bag 255 = equipment, 1-based slot).
local cache = {}
local pendingDataReq = {}
-- Items addressed by GUID counter (chat links, trade): peekCache[counter].
local peekCache, pendingPeekReq = {}, {}
-- Inspected players: inspectCache["name:slot"].
local inspectCache, pendingInspectReq = {}, {}
-- Auction listings: auctionCache["type:index"].
local auctionCache, pendingAuctionReq, auctionGroupOffset = {}, {}, {}
-- Trade slots resolved to GUID counters by the server.
local tradeCounter, selfTradeCounter = {}, {}
local pendingTradeReq, pendingSelfTradeReq = {}, {}

-- What the main tooltip shows right now, so an arriving reply can refresh it in place.
local current = {}

local function Debug(msg)
    if AFX_DEBUG then print("|cff44DDFF[ItemAffixes]|r " .. msg) end
end

local function Throttled(pending, key)
    local now = GetTime()
    local last = pending[key]
    if last and (now - last) < REQUEST_COOLDOWN then return true end
    pending[key] = now
    return false
end

-- uniqueId (GUID counter) from a 3.3.5a item link: item:id:enchant:gem1-4:suffix:uniqueId
local function GetItemUniqueId(link)
    if not link then return nil end
    return tonumber(link:match("|Hitem:%-?%d+:%-?%d+:%-?%d+:%-?%d+:%-?%d+:%-?%d+:%-?%d+:(%d+)"))
end

local function ItemIdFromLink(link)
    return link and tonumber(link:match("|Hitem:(%d+):"))
end

function AFXM:SendToServer(msg)
    SendAddonMessage(PREFIX, msg, "WHISPER", UnitName("player"))
end

function AFXM:RequestAllData()
    AFXM:SendToServer("ALLDATA")
end

-- ============================================================================
-- Rendering
-- ============================================================================

local function ParseSlots(parts, first, slotCount)
    local data = { slotCount = slotCount or 0, slots = {} }
    for i = first, #parts do
        local idx, state, text = parts[i]:match("^s(%d+):(%a):(.*)")
        if idx then
            data.slots[tonumber(idx) + 1] = { state = state, text = text }
        end
    end
    return data
end

-- Appends the affix lines once per tooltip content (tooltip._affixLinesKey guards repeats).
local function RenderAffixes(tooltip, data, renderKey)
    if not data or data.slotCount == 0 then return end
    if tooltip._affixLinesKey == renderKey then return end
    tooltip._affixLinesKey = renderKey

    local added = false
    for i = 1, data.slotCount do
        local s = data.slots[i]
        if s and s.state == "A" and s.text ~= "" then
            if not added then tooltip:AddLine(" "); added = true end
            if s.text:sub(1, 1) == "!" then
                tooltip:AddLine(COLOR_CRIT .. s.text:sub(2) .. "|r", 1, 0.84, 0)
            else
                tooltip:AddLine(COLOR_AFFIX .. s.text .. "|r", 0.27, 0.87, 1)
            end
        end
    end
    if added then tooltip:Show() end
end

local function ShowWaiting(tooltip)
    if tooltip == GameTooltip then
        tooltip:AddLine(COLOR_WAIT .. "[Fetching affixes...]|r")
        tooltip:Show()
    end
end

local function AddOwnLines(tooltip, bag, slot)
    if bag == 255 then
        if not GetInventoryItemLink("player", slot) then return end
    elseif not GetContainerItemLink(bag, slot) then
        return
    end
    local data = cache[bag] and cache[bag][slot]
    if not data then
        if not Throttled(pendingDataReq, bag .. ":" .. slot) then
            AFXM:SendToServer("DATA|" .. bag .. "|" .. slot)
        end
        ShowWaiting(tooltip)
        return
    end
    RenderAffixes(tooltip, data, "own:" .. bag .. ":" .. slot)
end

local function AddPeekLines(tooltip, uid)
    if not uid or uid == 0 then return end
    local data = peekCache[uid]
    if not data then
        if not Throttled(pendingPeekReq, uid) then
            AFXM:SendToServer("PEEK|" .. uid)
        end
        ShowWaiting(tooltip)
        return
    end
    RenderAffixes(tooltip, data, "peek:" .. uid)
end

local function AddInspectLines(tooltip, unit, slot)
    local name = UnitName(unit)
    if not name then return end
    local key = name .. ":" .. slot
    local data = inspectCache[key]
    if not data then
        if not Throttled(pendingInspectReq, key) then
            AFXM:SendToServer("PEEKUNIT|" .. name .. "|" .. slot)
        end
        ShowWaiting(tooltip)
        return
    end
    RenderAffixes(tooltip, data, "inspect:" .. key)
end

local function AddAuctionLines(tooltip, auctionType, index)
    local link = GetAuctionItemLink(auctionType, index)
    local uid = GetItemUniqueId(link)
    if uid and uid ~= 0 then
        AddPeekLines(tooltip, uid)
        return
    end
    -- AH links carry no GUID: the server finds the listing by seller, item and buyout.
    local key = auctionType .. ":" .. index
    local data = auctionCache[key]
    if not data then
        local info = { GetAuctionItemInfo(auctionType, index) }
        local owner, buyout, itemId = info[12], info[9] or 0, ItemIdFromLink(link)
        if owner and itemId and not Throttled(pendingAuctionReq, key) then
            AFXM:SendToServer("PEEKAUCTION|" .. owner .. "|" .. itemId .. "|" .. buyout
                .. "|" .. auctionType .. "|" .. index .. "|" .. (auctionGroupOffset[key] or 0))
        end
        ShowWaiting(tooltip)
        return
    end
    RenderAffixes(tooltip, data, "ah:" .. key)
end

-- Several copies of one item from one seller at one price: the n-th listing is the n-th item.
local function RebuildAuctionGroupOffsets(auctionType)
    local count = GetNumAuctionItems and GetNumAuctionItems(auctionType) or 0
    local seen = {}
    for i = 1, count do
        local info = { GetAuctionItemInfo(auctionType, i) }
        local itemId = ItemIdFromLink(GetAuctionItemLink(auctionType, i))
        if info[12] and itemId then
            local group = info[12] .. ":" .. itemId .. ":" .. (info[9] or 0)
            auctionGroupOffset[auctionType .. ":" .. i] = seen[group] or 0
            seen[group] = (seen[group] or 0) + 1
        end
    end
end

-- Trade links carry no reliable GUID; the server resolves the trade slot itself.
local function AddTradeLines(tooltip, tradeSlot, isPlayer)
    local counters = isPlayer and selfTradeCounter or tradeCounter
    local counter = counters[tradeSlot]
    if counter then
        AddPeekLines(tooltip, counter)
        return
    end
    local pending = isPlayer and pendingSelfTradeReq or pendingTradeReq
    if not Throttled(pending, tradeSlot) then
        AFXM:SendToServer((isPlayer and "MYTRADEPEEK|" or "TRADEPEEK|") .. tradeSlot)
    end
end

-- ============================================================================
-- Incoming messages
-- ============================================================================

local function RefreshMainTooltip(kind)
    if not GameTooltip:IsVisible() or current.kind ~= kind then return end
    pcall(function()
        if kind == "own" then
            if current.bag == 255 then
                GameTooltip:SetInventoryItem("player", current.slot)
            else
                GameTooltip:SetBagItem(current.bag, current.slot)
            end
        elseif kind == "inspect" then
            GameTooltip:SetInventoryItem(current.unit, current.slot)
        elseif kind == "auction" then
            GameTooltip:SetAuctionItem(current.auctionType, current.index)
        elseif kind == "trade" then
            if current.isPlayer then
                GameTooltip:SetTradePlayerItem(current.slot)
            else
                GameTooltip:SetTradeTargetItem(current.slot)
            end
        elseif kind == "link" then
            GameTooltip:SetHyperlink(current.link)
        end
    end)
end

local handlers = {}

handlers.DATA = function(parts)
    local bag, slot, count = tonumber(parts[2]), tonumber(parts[3]), tonumber(parts[4])
    if not bag or not slot or not count then return end
    cache[bag] = cache[bag] or {}
    cache[bag][slot] = ParseSlots(parts, 5, count)
    pendingDataReq[bag .. ":" .. slot] = nil
    if current.kind == "own" and current.bag == bag and current.slot == slot then
        RefreshMainTooltip("own")
    end
    if bag == 255 then
        for _, name in ipairs({ "ShoppingTooltip1", "ShoppingTooltip2" }) do
            local tt = _G[name]
            if tt and tt:IsShown() and tt._affixCompareEquipSlot == slot then
                tt._affixLinesKey = nil
                AddOwnLines(tt, 255, slot)
            end
        end
    end
end

handlers.PEEKDATA = function(parts)
    local uid, count = tonumber(parts[2]), tonumber(parts[3])
    if not uid or not count then return end
    peekCache[uid] = ParseSlots(parts, 4, count)
    RefreshMainTooltip("link")
    RefreshMainTooltip("auction")
end

handlers.INSPECTDATA = function(parts)
    local name, slot, count = parts[2], tonumber(parts[3]), tonumber(parts[4])
    if not name or not slot or not count then return end
    inspectCache[name .. ":" .. slot] = ParseSlots(parts, 5, count)
    RefreshMainTooltip("inspect")
end

handlers.AUCTIONDATA = function(parts)
    local auctionType, index, count = parts[5], tonumber(parts[6]), tonumber(parts[7])
    if not auctionType or not index or not count then return end
    auctionCache[auctionType .. ":" .. index] = ParseSlots(parts, 8, count)
    RefreshMainTooltip("auction")
end

local function HandleTradeGuid(parts, isPlayer)
    local tradeSlot, counter, count = tonumber(parts[2]), tonumber(parts[3]), tonumber(parts[4])
    if not tradeSlot or not counter or counter == 0 then return end
    local counters = isPlayer and selfTradeCounter or tradeCounter
    counters[tradeSlot] = counter
    peekCache[counter] = ParseSlots(parts, 5, count or 0)
    if current.kind == "trade" and current.isPlayer == isPlayer and current.slot == tradeSlot then
        RefreshMainTooltip("trade")
    end
end
handlers.TRADEGUID   = function(parts) HandleTradeGuid(parts, false) end
handlers.MYTRADEGUID = function(parts) HandleTradeGuid(parts, true) end

handlers.CONFIG = function() end  -- D3 mode only; nothing to configure client-side

handlers.ERR = function(parts)
    print("|cff44DDFF[ItemAffixes]|r " .. (parts[4] or "Unknown error"))
end

handlers.REFORGE_OPEN = function()
    if AFXM.ShowReforgeFrame then AFXM:ShowReforgeFrame() end
end

handlers.REFORGESTATUS = function(parts)
    local bag, slot, numSlots = tonumber(parts[2]), tonumber(parts[3]), tonumber(parts[4])
    local lockedSlot, cost = tonumber(parts[5]), tonumber(parts[6])
    if not (bag and slot and numSlots and lockedSlot and cost) then return end
    local lines = {}
    for i = 1, numSlots do lines[i] = parts[6 + i] end
    AFXM:HandleReforgeStatus(bag, slot, numSlots, lockedSlot, cost, lines)
end

handlers.REFORGEOPTS = function(parts)
    local bag, slot, affixSlot = tonumber(parts[2]), tonumber(parts[3]), tonumber(parts[4])
    if not (bag and slot and affixSlot) then return end
    local texts = {}
    for i = 5, #parts do texts[#texts + 1] = parts[i] end
    AFXM:HandleReforgeOpts(bag, slot, affixSlot, texts)
end

handlers.REFORGEPREVIEW = function(parts)
    local bag, slot, affixSlot, total = tonumber(parts[2]), tonumber(parts[3]), tonumber(parts[4]), tonumber(parts[5])
    if bag and slot and affixSlot and total then
        AFXM:HandleReforgePreviewHeader(bag, slot, affixSlot, total)
    end
end

handlers.REFORGEPREVIEWDATA = function(parts)
    local bag, slot, affixSlot = tonumber(parts[2]), tonumber(parts[3]), tonumber(parts[4])
    local chunk, total = tonumber(parts[5]), tonumber(parts[6])
    if not (bag and slot and affixSlot and chunk and total) then return end
    local texts = {}
    for i = 7, #parts do texts[#texts + 1] = parts[i] end
    AFXM:HandleReforgePreviewData(bag, slot, affixSlot, chunk, total, texts)
end

-- Public: the optional ItemAffixesToast addon hooks this with hooksecurefunc.
function AFXM:OnServerMsg(msg)
    if msg:sub(1, 5) == "AFXM\t" then msg = msg:sub(6) end
    local parts = {}
    for part in msg:gmatch("[^|]+") do parts[#parts + 1] = part end
    local handler = parts[1] and handlers[parts[1]]
    if handler then
        Debug("<- " .. msg)
        handler(parts)
    end
end

-- ============================================================================
-- Tooltip hooks
-- ============================================================================

local function SetCurrent(state)
    current = state
end

local INVTYPE_TO_SLOTS = {
    INVTYPE_HEAD = {1}, INVTYPE_NECK = {2}, INVTYPE_SHOULDER = {3}, INVTYPE_BODY = {4},
    INVTYPE_CHEST = {5}, INVTYPE_ROBE = {5}, INVTYPE_WAIST = {6}, INVTYPE_LEGS = {7},
    INVTYPE_FEET = {8}, INVTYPE_WRIST = {9}, INVTYPE_HAND = {10}, INVTYPE_FINGER = {11, 12},
    INVTYPE_TRINKET = {13, 14}, INVTYPE_CLOAK = {15}, INVTYPE_2HWEAPON = {16},
    INVTYPE_WEAPON = {16, 17}, INVTYPE_SHIELD = {17}, INVTYPE_WEAPONMAINHAND = {16},
    INVTYPE_WEAPONOFFHAND = {17}, INVTYPE_HOLDABLE = {17}, INVTYPE_RANGED = {18},
    INVTYPE_RANGEDRIGHT = {18}, INVTYPE_THROWN = {18}, INVTYPE_RELIC = {18}, INVTYPE_TABARD = {19},
}

local function CurrentLink()
    if current.kind == "own" then
        if current.bag == 255 then return GetInventoryItemLink("player", current.slot) end
        return GetContainerItemLink(current.bag, current.slot)
    elseif current.kind == "inspect" then
        return GetInventoryItemLink(current.unit, current.slot)
    elseif current.kind == "auction" then
        return GetAuctionItemLink(current.auctionType, current.index)
    elseif current.kind == "trade" then
        return current.isPlayer and GetTradePlayerItemLink(current.slot) or GetTradeTargetItemLink(current.slot)
    elseif current.kind == "link" then
        return current.link
    end
end

-- Comparison tooltips (ShoppingTooltip1/2) are filled in C by ShowCompareItem; append the
-- affixes of the equipped item(s) they show afterwards.
local function AddCompareAffixes()
    if current.kind == "own" and current.bag == 255 then return end
    local link = CurrentLink()
    if not link then return end
    local equipSlot = select(9, GetItemInfo(link))
    local slots = equipSlot and INVTYPE_TO_SLOTS[equipSlot]
    if not slots then return end
    for i, invSlot in ipairs(slots) do
        local tt = _G["ShoppingTooltip" .. i]
        if tt and tt:IsShown() then
            local resolved = invSlot
            if not GetInventoryItemLink("player", resolved) then
                resolved = (invSlot == 17 and GetInventoryItemLink("player", 16) and 16)
                    or (invSlot == 16 and GetInventoryItemLink("player", 17) and 17) or nil
            end
            if resolved then
                tt._affixCompareEquipSlot = resolved
                tt._affixLinesKey = nil
                AddOwnLines(tt, 255, resolved)
            end
        end
    end
end

local function HookTooltip(tt, isMain)
    hooksecurefunc(tt, "SetBagItem", function(self, bag, slot)
        self._affixLinesKey = nil
        if isMain then SetCurrent({ kind = "own", bag = bag, slot = slot }) end
        AddOwnLines(self, bag, slot)
    end)
    hooksecurefunc(tt, "SetInventoryItem", function(self, unit, slot)
        self._affixLinesKey = nil
        if unit == "player" then
            if isMain then SetCurrent({ kind = "own", bag = 255, slot = slot }) end
            AddOwnLines(self, 255, slot)
        elseif UnitIsPlayer(unit) then
            if isMain then SetCurrent({ kind = "inspect", unit = unit, slot = slot }) end
            AddInspectLines(self, unit, slot)
        end
    end)
    hooksecurefunc(tt, "SetHyperlink", function(self, link)
        self._affixLinesKey = nil
        if isMain then SetCurrent({ kind = "link", link = link }) end
        local uid = GetItemUniqueId(link)
        if not uid or uid == 0 then return end
        for slot = 1, 19 do
            if GetItemUniqueId(GetInventoryItemLink("player", slot)) == uid then
                AddOwnLines(self, 255, slot)
                return
            end
        end
        AddPeekLines(self, uid)
    end)
end

local hooked = {}

local function TryHookFrames()
    if not hooked.inspect and InspectFrame then
        hooked.inspect = true
        InspectFrame:HookScript("OnShow", function(self)
            local name = (self.unit and UnitName(self.unit)) or UnitName("target")
            if not name or name == UnitName("player") then return end
            for key in pairs(inspectCache) do
                if key:sub(1, #name + 1) == name .. ":" then inspectCache[key] = nil end
            end
            AFXM:SendToServer("PEEKUNITALL|" .. name)
        end)
    end
    if not hooked.compare and _G["GameTooltip_ShowCompareItem"] then
        hooked.compare = true
        hooksecurefunc("GameTooltip_ShowCompareItem", AddCompareAffixes)
    end
    for _, name in ipairs({ "ShoppingTooltip1", "ShoppingTooltip2", "ItemRefTooltip" }) do
        local tt = _G[name]
        if tt and not hooked[name] then
            hooked[name] = true
            pcall(HookTooltip, tt, false)
        end
    end
end

local function Init()
    HookTooltip(GameTooltip, true)
    pcall(function()
        hooksecurefunc(GameTooltip, "SetAuctionItem", function(self, auctionType, index)
            self._affixLinesKey = nil
            SetCurrent({ kind = "auction", auctionType = auctionType, index = index })
            AddAuctionLines(self, auctionType, index)
        end)
    end)
    pcall(function()
        hooksecurefunc(GameTooltip, "SetTradePlayerItem", function(self, tradeSlot)
            self._affixLinesKey = nil
            SetCurrent({ kind = "trade", isPlayer = true, slot = tradeSlot })
            AddTradeLines(self, tradeSlot, true)
        end)
    end)
    pcall(function()
        hooksecurefunc(GameTooltip, "SetTradeTargetItem", function(self, tradeSlot)
            self._affixLinesKey = nil
            SetCurrent({ kind = "trade", isPlayer = false, slot = tradeSlot })
            AddTradeLines(self, tradeSlot, false)
        end)
    end)
    GameTooltip:HookScript("OnHide", function() SetCurrent({}) end)
    TryHookFrames()
end

-- ============================================================================
-- Events
-- ============================================================================

local function ClearBagCache()
    for bag in pairs(cache) do
        if bag ~= 255 then cache[bag] = nil end
    end
    pendingDataReq = {}
end

local function ClearTrade()
    tradeCounter, selfTradeCounter, pendingTradeReq, pendingSelfTradeReq = {}, {}, {}, {}
end

local function ClearAuctions()
    auctionCache, pendingAuctionReq, auctionGroupOffset = {}, {}, {}
end

local events = {}

events.ADDON_LOADED = function(name)
    if name == ADDON_NAME then Init() end
    TryHookFrames()  -- Blizzard_InspectUI and friends load on demand
end

events.CHAT_MSG_ADDON = function(prefix, msg)
    if prefix == PREFIX then AFXM:OnServerMsg(msg) end
end

events.PLAYER_ENTERING_WORLD = function()
    cache, inspectCache, pendingInspectReq = {}, {}, {}
    ClearAuctions()
    AFXM:SendToServer("CONFIG")
    AFXM:RequestAllData()
end

-- Positions change on every move: drop position-keyed data and refetch once the move landed.
events.ITEM_LOCK_CHANGED = ClearBagCache
events.BAG_UPDATE_DELAYED = function()
    ClearBagCache()
    AFXM:RequestAllData()
end

events.PLAYER_EQUIPMENT_CHANGED = function(slot)
    if slot and cache[255] then cache[255][slot] = nil end
    ClearBagCache()
    AFXM:RequestAllData()
end

events.TRADE_TARGET_ITEM_CHANGED = function(slot)
    if type(slot) == "number" then
        tradeCounter[slot], selfTradeCounter[slot] = nil, nil
        pendingTradeReq[slot], pendingSelfTradeReq[slot] = nil, nil
    else
        ClearTrade()
    end
end
events.TRADE_CLOSED = ClearTrade

events.AUCTION_ITEM_LIST_UPDATE = function()
    ClearAuctions()
    RebuildAuctionGroupOffsets("list")
    RebuildAuctionGroupOffsets("bidder")
    RebuildAuctionGroupOffsets("owner")
end
events.AUCTION_HOUSE_CLOSED = ClearAuctions

local frame = CreateFrame("Frame")
for event in pairs(events) do frame:RegisterEvent(event) end
frame:SetScript("OnEvent", function(_, event, ...) events[event](...) end)

-- ============================================================================
-- Slash commands
-- ============================================================================

SLASH_AFXREFORGE1 = "/reforge"
SlashCmdList["AFXREFORGE"] = function()
    if AFXM.ShowReforgeFrame then AFXM:ShowReforgeFrame() end
end

SLASH_ITEMAFFIXES1 = "/affixes"
SlashCmdList["ITEMAFFIXES"] = function(msg)
    if msg == "debug" then
        AFX_DEBUG = not AFX_DEBUG
        print("|cff44DDFF[ItemAffixes]|r Debug " .. (AFX_DEBUG and "ON" or "OFF"))
        return
    end
    if msg == "refresh" then
        cache = {}
        AFXM:RequestAllData()
        print("|cff44DDFF[ItemAffixes]|r Refreshing affix data.")
        return
    end
    local count = 0
    for _, slots in pairs(cache) do
        for _ in pairs(slots) do count = count + 1 end
    end
    print("|cff44DDFF[ItemAffixes]|r " .. count .. " item(s) cached. /affixes debug | refresh")
end
