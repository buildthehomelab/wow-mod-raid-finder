-- Exercise the real addon under a minimal 3.3.5-style frame/event API, inside a stand-in
-- Raid Browser (LFRParentFrame with its two stock tabs).
local frames, buttons, sent = {}, {}, {}
local methods = {}
function methods:SetScript(name, fn) self.scripts[name] = fn end
function methods:SetText(text) self.text = text; if self.kind == 'Button' and self.bar then buttons[text] = self end end
function methods:GetText() return self.text end
function methods:Show() local was = self.shown; self.shown = true; if not was and self.scripts.OnShow then self.scripts.OnShow(self) end end
function methods:Hide() self.shown = false end
function methods:IsShown() return self.shown end
function methods:Enable() self.enabled = true end
function methods:Disable() self.enabled = false end
function methods:SetPoint(...) self.point = { ... } end
local function region(_, name)
    local r = setmetatable({ scripts = {} }, { __index = methods })
    if name then _G[name] = r end
    return r
end
methods.CreateFontString, methods.CreateTexture = region, region
setmetatable(methods, { __index = function() return function() end end })
function CreateFrame(kind, name, parent, template)
    local f = setmetatable({ kind = kind, name = name, parent = parent, template = template, scripts = {}, enabled = true }, { __index = methods })
    -- The bottom bar's buttons are the panel's UIPanelButtonTemplate2 children.
    f.bar = kind == 'Button' and template == 'UIPanelButtonTemplate2'
    -- The real template's OnShow/OnEnable/OnDisable concatenate self:GetName().
    assert(not f.bar or name, 'UIPanelButtonTemplate2 needs a frame name')
    frames[#frames + 1] = f
    if name then _G[name] = f end
    return f
end
UIParent = {}; UISpecialFrames = {}; StaticPopupDialogs = {}; SlashCmdList = {}; hash_SlashCmdList = {}; CANCEL = 'Cancel'
LFRParentFrame = CreateFrame('Frame', 'LFRParentFrame')
CreateFrame('Button', 'LFRParentFrameTab2', LFRParentFrame)
CreateFrame('Frame', 'LFRQueueFrame', LFRParentFrame)
CreateFrame('Frame', 'LFRBrowseFrame', LFRParentFrame)
local numTabs, selectedTab
function PanelTemplates_SetNumTabs(_, n) numTabs = n end
function PanelTemplates_SetTab(_, n) selectedTab = n end
-- The stock function, which only knows tabs 1 and 2.
function LFRFrame_SetActiveTab(tab)
    if tab == 1 then LFRParentFrame.activeTab = 1; LFRQueueFrame:Show(); LFRBrowseFrame:Hide()
    elseif tab == 2 then LFRParentFrame.activeTab = 2; LFRBrowseFrame:Show(); LFRQueueFrame:Hide() end
    PanelTemplates_SetTab(LFRParentFrame, tab)
end
function hooksecurefunc(name, hook)
    local original = _G[name]
    _G[name] = function(...) original(...); hook(...) end
end
function ShowUIPanel(f) f:Show() end
function HideUIPanel(f) f:Hide() end
function PlaySound() end
local scrollOffset = 0
function FauxScrollFrame_GetOffset() return scrollOffset end
function FauxScrollFrame_Update() end
function FauxScrollFrame_OnVerticalScroll() end
local instanceType = 'none'
function IsInInstance() return instanceType ~= 'none', instanceType end
local now = 0
local lastNonce
function GetTime() return now end
function GetNumRaidMembers() return 40 end
function UnitName() return 'Tester' end
function SendChatMessage(text, channel)
    local raw = text
    if string.find(text, '^%.raidfinder ') then
        lastNonce = string.match(text, ' (%d+)$')
        assert(lastNonce, 'finder commands must carry request IDs')
        text = string.gsub(text, ' %d+$', '')
        text = string.gsub(text, ' catalog %-$', ' catalog')
    end
    sent[#sent + 1] = { text, channel, raw }
end
function SendAddonMessage(prefix, text, channel) sent[#sent + 1] = { prefix .. ':' .. text, channel } end
function date(_, n) return tostring(n) end
local popup
function StaticPopup_Show(which) popup = { which = which }; return popup end
function StaticPopup_Hide() popup = nil end
dofile('addon/RaidFinder/Model.lua')
dofile('addon/RaidFinder/RaidFinder.lua')
LFRFrame_SetActiveTab(1)
local panel = RaidFinderPanel
local events = frames[#frames]
local function message(text, sender, nonce)
    text = "3\t" .. (nonce or lastNonce) .. "\t" .. string.sub(text, 3)
    events.scripts.OnEvent(events, 'CHAT_MSG_ADDON', 'RAIDFINDER', text, 'WHISPER', sender or 'Tester')
end
local function catalogue(...)
    message('1\tBEGIN')
    for _, line in ipairs({ ... }) do message('1\tRAID\t' .. line) end
    message('1\tEND')
end
local MC = 'mc\tMolten Core\t409\t0\t60\t40\t5\t11\t24\t1'
local BWL = 'bwl\tBlackwing Lair\t469\t0\t60\t40\t5\t13\t22\t1'

-- /lfr opens the Raid Browser on our tab; the stock frames hide, the catalogue is asked for.
assert(numTabs == 3 and LFRParentFrameTab3 and LFRParentFrameTab3.text == 'Raid Finder')
assert(hash_SlashCmdList['/LFR'] == SlashCmdList.RAIDFINDER, '/lfr must open Raid Finder, not the Raid Browser')
assert(not panel.shown and LFRQueueFrame.shown)
SlashCmdList.RAIDFINDER()
assert(LFRParentFrame.shown and panel.shown and not LFRQueueFrame.shown and not LFRBrowseFrame.shown and selectedTab == 3)
assert(sent[#sent][1] == '.raidfinder catalog')
LFRFrame_SetActiveTab(1)
assert(not panel.shown and LFRQueueFrame.shown, 'the stock tabs still work')
LFRParentFrameTab3.scripts.OnClick(LFRParentFrameTab3)
assert(panel.shown and not LFRQueueFrame.shown)
-- Re-showing the tab invalidated the catalogue in flight: its reply is stale, a new one follows.
catalogue(MC)
assert(not RaidFinderPanelListButton1.shown, 'a catalogue from before the tab was re-shown is stale')
panel.scripts.OnUpdate(panel, 0)
assert(sent[#sent][1] == '.raidfinder catalog')
catalogue(MC, BWL, 'naxx10\tNaxxramas\t533\t2\t80\t10\t2\t2\t6\t1', 'naxx25\tNaxxramas\t533\t2\t80\t25\t3\t5\t17\t1')
assert(sent[#sent][1] == '.raidfinder status mc')
-- Headers over their raids; same-named raids show their size.
assert(RaidFinderPanelListButton1.header == 0 and RaidFinderPanelListButton2.alias == 'mc')
assert(RaidFinderPanelListButton4.header == 2 and RaidFinderPanelListButton5.alias == 'naxx10')
assert(RaidFinderPanelListButton5.name.text == 'Naxxramas (10)' and RaidFinderPanelListButton2.name.text == 'Molten Core')
RaidFinderPanelListButton1.scripts.OnClick(RaidFinderPanelListButton1)
assert(RaidFinderPanelListButton2.header == 2, 'a collapsed header hides its raids')
RaidFinderPanelListButton1.scripts.OnClick(RaidFinderPanelListButton1)
assert(RaidFinderPanelListButton2.alias == 'mc')
message('1\tSTATUS\tmc\t1\t3\tNo personal raid save.\t1\tstone')
message('1\tRESULT\tstatus\tmc\t1\tReady')
assert(RaidFinderPanelRoleTank.count.text == '5' and RaidFinderPanelRoleDamage.mine.shown and not RaidFinderPanelRoleTank.mine.shown)
assert(RaidFinderPanelInfo.text:find('Roster 1 / 40'))
-- Recruit: Find Raid, Journal, Dismiss Bots.
assert(buttons['Find Raid'].enabled and buttons['Journal'] and buttons['Dismiss Bots'])
buttons['Find Raid'].scripts.OnClick(buttons['Find Raid'])
assert(sent[#sent][1] == '.raidfinder assemble mc')
message('1\tRESULT\tassemble\tmc\t1\tAssembled', 'OtherPlayer')
assert(not popup, 'other players cannot offer entry')
events.scripts.OnEvent(events, 'RAID_ROSTER_UPDATE')
message('1\tRESULT\tassemble\tmc\t0\tShort of healers')
assert(not popup, 'failed assembly must not offer travel')
now = now + 0.5; panel.scripts.OnUpdate(panel, 0.5)
catalogue(MC, BWL)
message('1\tRESULT\tstatus\tmc\t1\tReady')
assert(RaidFinderPanelNotice.text == 'Short of healers')
buttons['Find Raid'].scripts.OnClick(buttons['Find Raid'])
events.scripts.OnEvent(events, 'PARTY_MEMBERS_CHANGED')
message('1\tRESULT\tassemble\tmc\t1\tAssembled')
assert(not popup, 'revalidate the assembled roster before offering travel')
now = now + 0.5; panel.scripts.OnUpdate(panel, 0.5)
catalogue(MC, BWL)
message('1\tSTATUS\tmc\t40\t3\tNo personal raid save.\t1\tstone')
message('1\tRESULT\tstatus\tmc\t1\tReady')
assert(popup and popup.which == 'RAIDFINDER_ENTER')
-- Gather: Summon to Stone, Gather Bots, Dismiss Bots once the roster is full.
assert(buttons['Summon to Stone'].enabled and buttons['Gather Bots'].enabled)
StaticPopupDialogs.RAIDFINDER_ENTER.OnAccept(popup)
assert(sent[#sent][1] == '.raidfinder summon mc')
message('1\tRESULT\tsummon\tmc\t1\tSummoning')
events.scripts.OnEvent(events, 'PLAYER_ENTERING_WORLD')
local bar = buttons['Summon to Stone']
assert(bar.text == 'Find Raid' and not bar.enabled, 'nothing is offered until the catalogue is fresh')
now = now + 0.5; panel.scripts.OnUpdate(panel, 0.5)
catalogue(MC, BWL)
message('1\tSTATUS\tmc\t40\t3\tNo personal raid save.\t1\tstone')
message('1\tRESULT\tstatus\tmc\t1\tReady')
buttons['Gather Bots'].scripts.OnClick(buttons['Gather Bots'])
assert(sent[#sent][1] == '.raidfinder rally mc')
message('1\tRESULT\trally\tmc\t1\tArriving')
print('PASS: Raid Browser tab 3, /lfr, headers and sizes, roles, sender authentication, assembly failure/success, summon confirmation, rally')

-- Without the Legacy perk Summon to Stone stays visible but disabled, and isn't offered.
events.scripts.OnEvent(events, 'RAID_ROSTER_UPDATE')
now = now + 0.5; panel.scripts.OnUpdate(panel, 0.5)
catalogue(MC, BWL)
message('1\tSTATUS\tmc\t40\t3\tNo personal raid save.\t0\tstone')
message('1\tRESULT\tstatus\tmc\t1\tReady')
assert(not buttons['Summon to Stone'].enabled and buttons['Summon to Stone'].tip:find('Legacy perk Summoning Stone'))
assert(buttons['Gather Bots'].enabled)
print('PASS: no Legacy perk: Summon disabled with the perk named, Gather Bots still offered')

-- Run: inside a raid the bar becomes Start Run, Pause, Stop Run.
instanceType = 'raid'
events.scripts.OnEvent(events, 'PLAYER_ENTERING_WORLD')
now = now + 0.5; panel.scripts.OnUpdate(panel, 0.5)
catalogue(MC, BWL)
message('1\tSTATUS\tmc\t40\t3\tNo personal raid save.\t1\tstone')
message('1\tRESULT\tstatus\tmc\t1\tReady')
assert(buttons['Start Run'].enabled and buttons['Pause'] and buttons['Stop Run'])
buttons['Start Run'].scripts.OnClick(buttons['Start Run'])
message('1\tRESULT\tready\tmc\t0\tNot arrived')
assert(sent[#sent][1] == '.raidfinder ready mc', 'failed readiness must not start DC')
buttons['Start Run'].scripts.OnClick(buttons['Start Run'])
message('1\tRESULT\tready\tmc\t1\tReady')
assert(sent[#sent-1][1] == '.dc on')
local count = #sent
message('1\tRESULT\tready\tmc\t1\tReady')
assert(#sent == count, 'replay must not issue another DC command')
buttons['Start Run'].scripts.OnClick(buttons['Start Run'])
now = now + 11; panel.scripts.OnUpdate(panel, 11)
assert(buttons['Start Run'].enabled, 'timeout must unlock the UI')
instanceType = 'none'
print('PASS: run bar, readiness failure/success, replay, timeout')

-- An empty catalogue offers nothing to launch.
events.scripts.OnEvent(events, 'QUEST_LOG_UPDATE')
now = now + 11; panel.scripts.OnUpdate(panel, 11)
assert(sent[#sent][1] == '.raidfinder catalog')
catalogue()
assert(not buttons['Find Raid'].enabled and not RaidFinderPanelListButton1.shown)
-- An old END must not complete a newer catalogue after the first request times out.
local oldNonce = lastNonce
events.scripts.OnEvent(events, 'QUEST_LOG_UPDATE')
now = now + 0.5; panel.scripts.OnUpdate(panel, 0.5)
local staleNonce = lastNonce
now = now + 11; panel.scripts.OnUpdate(panel, 11)
events.scripts.OnEvent(events, 'QUEST_LOG_UPDATE')
now = now + 0.5; panel.scripts.OnUpdate(panel, 0.5)
assert(lastNonce ~= staleNonce and lastNonce ~= oldNonce)
message('1\tBEGIN', nil, staleNonce)
message('1\tRAID\t' .. BWL, nil, staleNonce)
message('1\tEND', nil, staleNonce)
assert(not RaidFinderPanelListButton1.shown, 'a timed-out catalogue cannot restore a raid')
catalogue()
assert(not buttons['Find Raid'].enabled)
print('PASS: empty catalogue cannot launch; timed-out catalogue packets are rejected')
