-- ============================================================================
-- ItemAffixesToast -- optional companion addon for ItemAffixes (originally by Rykaerdoe).
--
-- Adds:
--   1. A toast + sound when an item rolls a Critical affix (on pickup or in a Reforge).
--   2. A Character Pane refresh after equipment changes, so affix stats show at once.
--
-- Self-contained: hooks AFXM.OnServerMsg from the outside and parses the raw messages.
-- ============================================================================

if not AFXM then return end  -- ItemAffixes didn't load; nothing to hook

-- ----------------------------------------------------------------------------
-- Toast notifications
-- ----------------------------------------------------------------------------

local AFX_TOAST_SOUND_ID = 6555  -- shaybells; change if you'd prefer a different sound
local AFX_TOAST_HOLD_SEC = 2.2   -- how long the toast stays fully visible
local AFX_TOAST_FADE_SEC = 0.6   -- fade-out duration after the hold

local toastFrame   = nil
local toastQueue    = {}
local toastShowing  = false
local toastUpdater  = nil

local function GetToastFrame()
    if toastFrame then return toastFrame end

    local f = CreateFrame("Frame", "AFXToastFrame", UIParent)
    f:SetSize(380, 54)
    f:SetPoint("TOP", UIParent, "TOP", 0, -140)
    f:SetFrameStrata("HIGH")
    f:SetBackdrop({
        bgFile   = "Interface\\Tooltips\\UI-Tooltip-Background",
        edgeFile = "Interface\\Tooltips\\UI-Tooltip-Border",
        tile = true, tileSize = 16, edgeSize = 12,
        insets = { left = 3, right = 3, top = 3, bottom = 3 },
    })
    f:SetBackdropColor(0, 0, 0, 0.75)
    f:SetBackdropBorderColor(1, 0.82, 0, 1)

    local text = f:CreateFontString(nil, "OVERLAY", "GameFontNormalLarge")
    text:SetPoint("CENTER", f, "CENTER", 0, 0)
    text:SetJustifyH("CENTER")
    text:SetWidth(360)
    f._text = text

    f:SetAlpha(0)
    f:Hide()
    toastFrame = f
    return f
end

local function ShowNextToast()
    if toastShowing or #toastQueue == 0 then return end
    toastShowing = true

    local entry = table.remove(toastQueue, 1)
    local f = GetToastFrame()
    f._text:SetText(entry.text)
    f:Show()
    f:SetAlpha(0)
    UIFrameFadeIn(f, 0.35, 0, 1)

    if entry.sound then
        PlaySound(AFX_TOAST_SOUND_ID)
    end

    local elapsed = 0
    local fadeStarted = false
    if not toastUpdater then
        toastUpdater = CreateFrame("Frame")
    end
    toastUpdater:SetScript("OnUpdate", function(self, dt)
        elapsed = elapsed + dt
        if not fadeStarted and elapsed >= AFX_TOAST_HOLD_SEC then
            fadeStarted = true
            UIFrameFadeOut(f, AFX_TOAST_FADE_SEC, 1, 0)
        end
        if elapsed >= (AFX_TOAST_HOLD_SEC + AFX_TOAST_FADE_SEC) then
            self:SetScript("OnUpdate", nil)
            f:Hide()
            toastShowing = false
            ShowNextToast()
        end
    end)
end

local function ShowToast(text, playSound)
    toastQueue[#toastQueue + 1] = { text = text, sound = (playSound ~= false) }
    ShowNextToast()
end


-- ----------------------------------------------------------------------------
-- D3ROLL|bag|slot|!text -- the server rolled a crit affix on a newly received item.
-- REFORGEOPTS|bag|slot|affixSlot|current|opt1|... -- text 5 is the current value, so the
-- scan starts at 6: rerolling an item that already had a crit does not toast again.
-- ----------------------------------------------------------------------------

local function ToastCrits(parts, first)
    for i = first, #parts do
        if parts[i]:sub(1, 1) == "!" then
            ShowToast("|cffFFD700Critical Roll!|r\n" .. parts[i]:sub(2))
        end
    end
end

hooksecurefunc(AFXM, "OnServerMsg", function(_, msg)
    if msg:sub(1, 5) == "AFXM\t" then msg = msg:sub(6) end
    local parts = {}
    for part in msg:gmatch("[^|]+") do parts[#parts + 1] = part end
    if parts[1] == "D3ROLL" then
        ToastCrits(parts, 4)
    elseif parts[1] == "REFORGEOPTS" then
        ToastCrits(parts, 6)
    end
end)

-- ----------------------------------------------------------------------------
-- CharPaneRefresh -- keeps the Character Pane's spell crit % display current.
--
-- The client only recomputes paper-doll stats on a narrow set of events
-- (UNIT_STATS, UNIT_ATTACK, etc.), not on every equipment change. A gear
-- swap that grants crit % without touching a tracked stat -- like a talent
-- affix on a plain bracer -- never triggers that recompute, so the panel
-- shows a stale value until something else happens to touch a tracked stat.
--
-- On PLAYER_EQUIPMENT_CHANGED, wait briefly (the server's own update needs
-- a moment to arrive) then force a recompute via PaperDollFrame_UpdateStats.
-- ----------------------------------------------------------------------------

local CHARPANE_REFRESH_DELAY = 0.1 -- seconds

local charPaneFrame = CreateFrame("Frame")
charPaneFrame:RegisterEvent("PLAYER_EQUIPMENT_CHANGED")

local charPaneRefreshPending = false
local charPaneElapsed = 0

charPaneFrame:SetScript("OnEvent", function()
    charPaneRefreshPending = true
    charPaneElapsed = 0
end)

charPaneFrame:SetScript("OnUpdate", function(self, elapsed)
    if not charPaneRefreshPending then return end
    charPaneElapsed = charPaneElapsed + elapsed
    if charPaneElapsed >= CHARPANE_REFRESH_DELAY then
        charPaneRefreshPending = false
        if PaperDollFrame and PaperDollFrame:IsVisible() and PaperDollFrame_UpdateStats then
            PaperDollFrame_UpdateStats()
        end
    end
end)

