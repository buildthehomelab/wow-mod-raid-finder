-- Raid Finder: a third tab in the stock Raid Browser (LFRParentFrame), laid out like Choose Raid.
-- Stock templates only; DragonUI, when installed, skins the window and our buttons.
local M = RaidFinderModel
local state = M.New()
local HEADERS = { [0] = "Classic Raid", [1] = "Burning Crusade Raid", [2] = "Lich King Raid" }
local ROWS, ROW_HEIGHT = 14, 16

local NineSliceUtils = _G.DragonUI_NineSlice or _G.NineSliceUtils
local function Dragon()
    local D = _G.DragonUI
    if D and D._dir and D.SkinRedButton and NineSliceUtils then return D end
end

local panel = CreateFrame("Frame", "RaidFinderPanel", LFRParentFrame)
panel:SetAllPoints(LFRParentFrame)
panel:Hide()
local layout = panel:CreateTexture("RaidFinderPanelLayout", "BACKGROUND")
layout:SetTexture("Interface\\LFGFrame\\UI-LFR-FRAME-MAIN")
layout:SetSize(512, 512)
layout:SetPoint("TOPLEFT")
local title = panel:CreateFontString("RaidFinderPanelTitleText", "ARTWORK", "GameFontNormal")
title:SetPoint("TOP", 12, -18)
title:SetText("Raid Finder")

-- The lineup's tanks, healers and damage, on the stock role icons; your own role is ticked.
local roleIcons = {}
local function RoleIcon(key, role, x, anchor)
    local b = CreateFrame("Button", "RaidFinderPanelRole" .. key, panel)
    b:SetSize(48, 48)
    if anchor then b:SetPoint("LEFT", anchor, "RIGHT", x, 0) else b:SetPoint("TOPLEFT", x, -60) end
    local bg = b:CreateTexture(nil, "BACKGROUND")
    bg:SetTexture("Interface\\LFGFrame\\UI-LFG-ICONS-ROLEBACKGROUNDS")
    bg:SetSize(80, 80); bg:SetPoint("CENTER"); bg:SetAlpha(0.6)
    if GetBackgroundTexCoordsForRole then bg:SetTexCoord(GetBackgroundTexCoordsForRole(role)) end
    local icon = b:CreateTexture(nil, "ARTWORK")
    icon:SetTexture("Interface\\LFGFrame\\UI-LFG-ICON-ROLES")
    icon:SetAllPoints()
    if GetTexCoordsForRole then icon:SetTexCoord(GetTexCoordsForRole(role)) end
    b.count = b:CreateFontString(nil, "OVERLAY", "GameFontHighlightLarge")
    b.count:SetPoint("BOTTOMRIGHT", 4, -2)
    b.mine = b:CreateTexture(nil, "OVERLAY")
    b.mine:SetTexture("Interface\\Buttons\\UI-CheckBox-Check")
    b.mine:SetSize(24, 24); b.mine:SetPoint("BOTTOMLEFT", -6, -6)
    b.label = key == "Tank" and "Tanks" or key == "Healer" and "Healers" or "Damage dealers"
    b:SetScript("OnEnter", function(self)
        GameTooltip:SetOwner(self, "ANCHOR_RIGHT")
        GameTooltip:SetText(self.label .. " in this raid's lineup: " .. (self.count:GetText() or "-"), 1, 1, 1)
        if self.mine:IsShown() then GameTooltip:AddLine("Your active spec plays this role.", nil, nil, nil, 1) end
        if key == "Tank" then GameTooltip:AddLine("A bot tank leads the run; human tanks aren't supported yet.", nil, nil, nil, 1) end
        GameTooltip:Show()
    end)
    b:SetScript("OnLeave", function() GameTooltip:Hide() end)
    roleIcons[key] = b
    return b
end
RoleIcon("Damage", "DAMAGER", 43, RoleIcon("Healer", "HEALER", 45, RoleIcon("Tank", "TANK", 82)))

-- The raid list: collapsible expansion headers over the raids, like Choose Raid.
local scroll = CreateFrame("ScrollFrame", "RaidFinderPanelListScrollFrame", panel, "FauxScrollFrameTemplate")
scroll:SetPoint("TOPLEFT", 25, -130)
scroll:SetSize(296, ROWS * ROW_HEIGHT + 2)
local top = scroll:CreateTexture(nil, "BACKGROUND")
top:SetTexture("Interface\\PaperDollInfoFrame\\UI-Character-ScrollBar")
top:SetSize(31, 200); top:SetPoint("TOPLEFT", scroll, "TOPRIGHT", -2, 5)
top:SetTexCoord(0, 0.484375, 0, 0.78125)
local bottom = scroll:CreateTexture(nil, "BACKGROUND")
bottom:SetTexture("Interface\\PaperDollInfoFrame\\UI-Character-ScrollBar")
bottom:SetSize(31, 106); bottom:SetPoint("BOTTOMLEFT", scroll, "BOTTOMRIGHT", -2, -2)
bottom:SetTexCoord(0.515625, 1.0, 0, 0.4140625)
local empty = panel:CreateFontString(nil, "ARTWORK", "GameFontNormal")
empty:SetWidth(300); empty:SetPoint("TOP", 0, -220)

-- The comment box of Choose Raid: the realm's answer, then roster, save and run state.
local notice = panel:CreateFontString("RaidFinderPanelNotice", "ARTWORK", "GameFontHighlightSmall")
notice:SetPoint("TOPLEFT", panel, "BOTTOMLEFT", 31, 116)
notice:SetSize(313, 28)
notice:SetJustifyH("LEFT"); notice:SetJustifyV("TOP")
local info = panel:CreateFontString("RaidFinderPanelInfo", "ARTWORK", "GameFontDisableSmall")
info:SetPoint("TOPLEFT", notice, "BOTTOMLEFT", 0, -2)
info:SetSize(313, 12)
info:SetJustifyH("LEFT")

local rows, buttons, list, collapsed = {}, {}, {}, {}
local Refresh, Request
local catalogDue = 0
local runText = ""
local requestId = math.floor(GetTime() * 1000) % 2147483646

local function Button(x, width, anchor)
    local b = CreateFrame("Button", nil, panel, "UIPanelButtonTemplate2")
    b:SetSize(width, 22)
    b:SetPoint(anchor, x, 12)
    b:SetScript("OnClick", function(self) if self.action then self.action() end end)
    b:SetScript("OnEnter", function(self)
        if not self.tip then return end
        GameTooltip:SetOwner(self, "ANCHOR_TOP")
        GameTooltip:SetText(self:GetText(), 1, 1, 1)
        GameTooltip:AddLine(self.tip, nil, nil, nil, 1)
        GameTooltip:Show()
    end)
    b:SetScript("OnLeave", function() GameTooltip:Hide() end)
    -- A disabled button still explains itself (the Legacy perk, for one).
    if b.SetMotionScriptsWhileDisabled then b:SetMotionScriptsWhileDisabled(true) end
    local D = Dragon()
    if D then D.SkinRedButton(b) end
    buttons[#buttons + 1] = b
    return b
end
-- The three stock button slots: List My Name, the middle slot and Set Comment.
local left, middle, right = Button(21, 125, "BOTTOMLEFT"), Button(147, 85, "BOTTOMLEFT"), Button(-7, 115, "BOTTOMRIGHT")

Request = function(action)
    if state.pending then return end
    if action ~= "catalog" and (not state.selected or state.catalogDirty) then return end
    if action == "catalog" then state.catalogDirty = true end
    if action == "assemble" or action == "summon" then state.assemblyResult = nil end
    requestId = requestId % 2147483646 + 1
    state.pending = { action = action, raid = state.selected, at = GetTime(), generation = state.catalogGeneration, id = requestId }
    state.notice = action == "assemble" and "Recruiting the raid..." or "Checking with the realm..."
    -- Native server commands also work while solo. Commands are consumed by the server.
    SendChatMessage(".raidfinder " .. action .. " " .. (action == "catalog" and "-" or state.selected) .. " " .. requestId, "SAY")
    Refresh()
end
StaticPopupDialogs.RAIDFINDER_ENTER = {
    text = "Summon to the meeting stone for %s? The leader also brings recruited bots. Each human confirms their own travel. Enter through the normal raid entrance.",
    button1 = "Summon to Stone", button2 = CANCEL, timeout = 0, whileDead = false, hideOnEscape = true,
    OnAccept = function(self)
        if self.data == state.selected then Request("summon") end
    end,
    OnCancel = function() state.assemblyResult = nil end,
}
StaticPopupDialogs.RAIDFINDER_DISMISS = {
    text = "Dismiss the bots recruited for you? Your friends and manually invited bots remain in the group.",
    button1 = "Dismiss Bots", button2 = CANCEL, timeout = 0, whileDead = false, hideOnEscape = true,
    OnAccept = function() SendChatMessage(".dc off", "SAY"); SendChatMessage(".botraid dismiss", "SAY") end,
}
local function OfferSummon()
    local raid = state.raids[state.selected]
    if not raid then return end
    local popup = StaticPopup_Show("RAIDFINDER_ENTER", M.DisplayName(state, state.selected))
    if popup then popup.data = state.selected end
end
local function OpenJournal()
    if Journal and Journal.OpenSection then Journal.OpenSection("raids")
    else state.notice = "Install the Journal addon to browse bosses and loot."; Refresh() end
end

-- Which three buttons the bar shows: recruit until the roster is full, then gather outside,
-- then run once you're inside a raid.
local function Phase(raid)
    local _, instanceType = IsInInstance()
    if instanceType == "raid" then return "run" end
    if raid and (state.members or 0) >= raid.size then return "gather" end
    return "recruit"
end
local function Set(b, text, action, enabled, tip)
    b:SetText(text); b.action = action; b.tip = tip
    if enabled then b:Enable() else b:Disable() end
end

local function RowClick(self)
    if self.header then
        collapsed[self.header] = not collapsed[self.header]
        Refresh()
        return
    end
    if state.pending or self.alias == state.selected then return end
    StaticPopup_Hide("RAIDFINDER_ENTER")
    state.assemblyResult = nil
    state.selected = self.alias
    state.members, state.role, state.save, state.perk, state.gather = nil, nil, nil, nil, nil
    runText = ""
    Request("status")
end
for i = 1, ROWS do
    local row = CreateFrame("Button", "RaidFinderPanelListButton" .. i, panel)
    row:SetSize(295, ROW_HEIGHT)
    row:SetPoint("TOPLEFT", 25, -132 - (i - 1) * ROW_HEIGHT)
    row:SetHighlightTexture("Interface\\QuestFrame\\UI-QuestTitleHighlight", "ADD")
    row.expand = row:CreateTexture(nil, "ARTWORK")
    row.expand:SetSize(14, 14); row.expand:SetPoint("LEFT", 2, 0)
    row.radio = row:CreateTexture(nil, "ARTWORK")
    row.radio:SetTexture("Interface\\Buttons\\UI-RadioButton")
    row.radio:SetSize(14, 14); row.radio:SetPoint("LEFT", 20, 0)
    row.name = row:CreateFontString(nil, "ARTWORK", "GameFontNormal")
    row.name:SetPoint("LEFT", 38, 0); row.name:SetJustifyH("LEFT")
    row.level = row:CreateFontString(nil, "ARTWORK", "GameFontNormal")
    row.level:SetPoint("RIGHT", -4, 0); row.level:SetJustifyH("RIGHT")
    row:SetScript("OnClick", RowClick)
    rows[i] = row
end
scroll:SetScript("OnVerticalScroll", function(self, offset)
    FauxScrollFrame_OnVerticalScroll(self, offset, ROW_HEIGHT, function() Refresh() end)
end)

local function UpdateList()
    for i = #list, 1, -1 do list[i] = nil end
    local last
    for _, alias in ipairs(state.order) do
        local raid = state.raids[alias]
        if raid.header ~= last then
            last = raid.header
            list[#list + 1] = { header = raid.header }
        end
        if not collapsed[raid.header] then list[#list + 1] = { alias = alias } end
    end
    local offset = FauxScrollFrame_GetOffset(scroll)
    for i = 1, ROWS do
        local row, entry = rows[i], (not state.catalogDirty) and list[i + offset]
        row.header, row.alias = nil, nil
        if not entry then
            row:Hide()
        elseif entry.header then
            row.header = entry.header
            row.expand:SetTexture(collapsed[entry.header] and "Interface\\Buttons\\UI-PlusButton-UP" or "Interface\\Buttons\\UI-MinusButton-UP")
            row.expand:Show(); row.radio:Hide()
            row.name:SetPoint("LEFT", 20, 0)
            row.name:SetText(HEADERS[entry.header] or "Raid")
            row.name:SetTextColor(1, 1, 1)
            row.level:SetText("")
            row:Show()
        else
            local raid = state.raids[entry.alias]
            row.alias = entry.alias
            row.expand:Hide(); row.radio:Show()
            if entry.alias == state.selected then row.radio:SetTexCoord(0.25, 0.5, 0, 1) else row.radio:SetTexCoord(0, 0.25, 0, 1) end
            row.name:SetPoint("LEFT", 38, 0)
            row.name:SetText(M.DisplayName(state, entry.alias) .. (raid.enabled and "" or "  [Locked]"))
            row.name:SetTextColor(1, 0.82, 0)
            row.level:SetText("(" .. raid.level .. ")")
            row:Show()
        end
    end
    FauxScrollFrame_Update(scroll, state.catalogDirty and 0 or #list, ROWS, ROW_HEIGHT)
    empty:SetText(state.catalogDirty and "Checking eligibility..." or (#state.order == 0 and "No raids are available for your progression, level and group." or ""))
end

Refresh = function()
    local raid = not state.catalogDirty and state.raids[state.selected]
    UpdateList()
    for key, b in pairs(roleIcons) do
        local n = raid and (key == "Tank" and raid.tanks or key == "Healer" and raid.healers or raid.damage)
        b.count:SetText(n and tostring(n) or "")
        local mine = state.role == 0 and key == "Tank" or state.role == 1 and key == "Healer" or (state.role == 2 or state.role == 3) and key == "Damage"
        if raid and mine then b.mine:Show() else b.mine:Hide() end
    end
    notice:SetText(state.notice)
    local save = state.save and string.gsub(state.save, "resets (%d+)", function(epoch) return "resets " .. date("%b %d, %H:%M", tonumber(epoch)) end)
    info:SetText(raid and string.format("Roster %d / %d  |  %s%s", state.members or 1, raid.size, save or "Checking your save...", runText) or "")
    local available = raid and raid.enabled and not state.pending
    local phase = Phase(raid)
    if phase == "run" then
        Set(left, "Start Run", function() Request("ready") end, available)
        Set(middle, "Pause", function() SendChatMessage(".dc pause", "SAY") end, raid and true)
        Set(right, "Stop Run", function() SendChatMessage(".dc off", "SAY") end, raid and true)
    elseif phase == "gather" then
        local where = state.gather == "entrance" and "the raid's entrance" or "the meeting stone"
        Set(left, "Summon to Stone", OfferSummon, available and state.perk ~= false,
            state.perk == false and ("Legacy perk Summoning Stone (Adventure). Without it, travel to " .. where .. " yourself, then use Gather Bots.")
            or ("Travel to " .. where .. " with your recruited bots."))
        Set(middle, "Gather Bots", function() Request("rally") end, available, "At " .. where .. ": bring your recruited bots to you.")
        Set(right, "Dismiss Bots", function() StaticPopup_Show("RAIDFINDER_DISMISS") end, true)
    else
        Set(left, "Find Raid", function() Request("assemble") end, available, "Fill the raid with level " .. (raid and raid.level or "-") .. " bots.")
        Set(middle, "Journal", OpenJournal, not state.pending, "Bosses and loot (Journal addon).")
        Set(right, "Dismiss Bots", function() StaticPopup_Show("RAIDFINDER_DISMISS") end, true)
    end
end

local elapsed = 0
panel:SetScript("OnUpdate", function(_, dt)
    elapsed = elapsed + dt
    if state.pending and GetTime() - state.pending.at > 10 then
        state.pending = nil
        catalogDue = math.huge
        state.notice = "No reply from Raid Finder. Check the module version and connection, then pick a raid again."
        Refresh()
    end
    if state.catalogDirty and not state.pending and GetTime() >= catalogDue then Request("catalog") end
    if elapsed > 5 and not state.pending and GetNumRaidMembers() > 0 then
        elapsed = 0
        SendAddonMessage("DC", "CMD\tstatus", "RAID")
    end
end)
panel:SetScript("OnShow", function() M.Invalidate(state); catalogDue = 0; Request("catalog") end)

-- The stock code only knows tabs 1 and 2; tab 3 hides both of their frames and shows ours.
local tab = CreateFrame("Button", "LFRParentFrameTab3", LFRParentFrame, "CharacterFrameTabButtonTemplate")
tab:SetID(3)
tab:SetText("Raid Finder")
tab:SetPoint("LEFT", LFRParentFrameTab2, "RIGHT", -15, 0)
tab:SetScript("OnClick", function() LFRFrame_SetActiveTab(3); PlaySound("igCharacterInfoTab") end)
tab:SetScript("OnEnter", function(self)
    GameTooltip:SetOwner(self, "ANCHOR_RIGHT")
    GameTooltip:SetText("Raid Finder", 1, 1, 1)
    GameTooltip:AddLine("Fill a raid with bots and gather at its meeting stone.", nil, nil, nil, 1)
    GameTooltip:Show()
end)
tab:SetScript("OnLeave", function() GameTooltip:Hide() end)
PanelTemplates_SetNumTabs(LFRParentFrame, 3)
hooksecurefunc("LFRFrame_SetActiveTab", function(which)
    if which == 3 then
        LFRParentFrame.activeTab = 3
        LFRQueueFrame:Hide()
        LFRBrowseFrame:Hide()
        panel:Show()
    else
        panel:Hide()
    end
end)
PanelTemplates_SetTab(LFRParentFrame, LFRParentFrame.activeTab or 1)

local events = CreateFrame("Frame")
events:RegisterEvent("CHAT_MSG_ADDON")
events:RegisterEvent("PLAYER_ENTERING_WORLD")
events:RegisterEvent("PARTY_MEMBERS_CHANGED")
events:RegisterEvent("RAID_ROSTER_UPDATE")
events:RegisterEvent("QUEST_LOG_UPDATE")
events:RegisterEvent("PLAYER_LEVEL_UP")
events:SetScript("OnEvent", function(_, event, prefix, message, channel, sender)
    if event ~= "CHAT_MSG_ADDON" then
        if event == "PLAYER_ENTERING_WORLD" then
            runText = ""
            state.pending = nil
        end
        M.Invalidate(state)
        StaticPopup_Hide("RAIDFINDER_ENTER")
        if not state.assemblyResult then state.notice = "Checking progression and group eligibility..." end
        catalogDue = GetTime() + 0.5
        Refresh()
        return
    end
    -- Finder packets are built by the server as whispers from ourselves.
    -- Never accept another player's packet as authority to issue a start command.
    if prefix == "RAIDFINDER" and channel == "WHISPER" and sender == UnitName("player") then
        local result = M.Receive(state, message)
        Refresh()
        if result == "catalog" and state.selected then Request("status") end
        if result == "assemble" then
            catalogDue = GetTime() + 0.5
        elseif result == "status" and state.assemblyResult and state.assemblyResult.success
            and state.assemblyResult.raid == state.selected and state.raids[state.selected] and state.perk ~= false then
            OfferSummon()
        elseif result == "ready" then
            SendChatMessage(".dc on", "SAY")
            SendAddonMessage("DC", "CMD\tstatus", "RAID")
            state.notice = "Start requested. Dungeon Clear will report the run state below."
            Refresh()
        end
    elseif prefix == "DC" and panel:IsShown() then
        local p = M.Split(message)
        if p[1] == "STATUS" then
            runText = "  |  Run: " .. (p[7] or "unknown")
            Refresh()
        elseif p[1] == "ERROR" then state.notice = p[2] or "Dungeon Clear error."; Refresh() end
    end
end)

-- /lfr (the client's own Raid Browser command) and /raidfinder open the Raid Browser on our tab.
local function Toggle()
    if LFRParentFrame:IsShown() and LFRParentFrame.activeTab == 3 then
        HideUIPanel(LFRParentFrame)
    else
        ShowUIPanel(LFRParentFrame)
        LFRFrame_SetActiveTab(3)
    end
end
SLASH_RAIDFINDER1 = "/lfr"
SLASH_RAIDFINDER2 = "/raidfinder"
SlashCmdList.RAIDFINDER = Toggle
-- The client already binds /lfr to its Raid Browser. ChatEdit_ParseText looks in
-- hash_SlashCmdList before it walks SlashCmdList, so claim /LFR there to win.
if hash_SlashCmdList then hash_SlashCmdList["/LFR"] = Toggle end
Refresh()
