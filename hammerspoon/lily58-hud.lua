-- lily58-hud.lua
-- Floating keyboard HUD for Lily58 ZMK layout
-- Place in ~/.hammerspoon/ and add: require("lily58-hud")

-- ─── Teardown guard (safe on reload) ─────────────────────────────────────────
if _G._lily58hud then
  pcall(function() _G._lily58hud.tap:stop() end)
  pcall(function() _G._lily58hud.modTap:stop() end)
  pcall(function() _G._lily58hud.wv:delete() end)
  pcall(function() _G._lily58hud.layerTimer:stop() end)
  pcall(function() _G._lily58hud.pending:stop() end)
end
_G._lily58hud = {}
local M = _G._lily58hud

-- ─── Constants ────────────────────────────────────────────────────────────────
local SETTINGS_MODE = "lily58hud.viewMode"
local SETTINGS_POS  = "lily58hud.position"
local HUD_W, HUD_H  = 340, 138

-- ─── Settings helpers ─────────────────────────────────────────────────────────
local function getMode() return hs.settings.get(SETTINGS_MODE) or "full" end
local function setMode(m) hs.settings.set(SETTINGS_MODE, m) end
local function getPos()   return hs.settings.get(SETTINGS_POS) end
local function setPos(x,y) hs.settings.set(SETTINGS_POS, {x=x, y=y}) end

-- ─── Keycode → DOM position ───────────────────────────────────────────────────
-- IDs: k-{L|R}-{row|T}-{col}   rows 0-3, thumbs = T, cols 0-6
-- For L2/L3 keys, maps to their physical ZMK position (not standard QWERTY pos)
local KEY_POS = {
  -- Row 0
  [53]="k-L-0-0",[18]="k-L-0-1",[19]="k-L-0-2",[20]="k-L-0-3",[21]="k-L-0-4",[23]="k-L-0-5",
  [22]="k-R-0-0",[26]="k-R-0-1",[28]="k-R-0-2",[25]="k-R-0-3",[29]="k-R-0-4",[50]="k-R-0-5",
  -- Row 1
  [48]="k-L-1-0",[12]="k-L-1-1",[13]="k-L-1-2",[14]="k-L-1-3",[15]="k-L-1-4",[17]="k-L-1-5",
  [16]="k-R-1-0",[32]="k-R-1-1",[34]="k-R-1-2",[31]="k-R-1-3",[35]="k-R-1-4",[27]="k-R-1-5",
  -- Row 2
  [57]="k-L-2-0",[0]="k-L-2-1", [1]="k-L-2-2", [2]="k-L-2-3", [3]="k-L-2-4", [5]="k-L-2-5",
  [4]="k-R-2-0", [38]="k-R-2-1",[40]="k-R-2-2",[37]="k-R-2-3",[41]="k-R-2-4",[39]="k-R-2-5",
  -- Row 3
  [59]="k-L-3-0",[6]="k-L-3-1", [7]="k-L-3-2", [8]="k-L-3-3", [9]="k-L-3-4", [11]="k-L-3-5",
  [45]="k-R-3-1",[46]="k-R-3-2",[43]="k-R-3-3",[47]="k-R-3-4",[44]="k-R-3-5",[60]="k-R-3-6",
  -- Thumbs
  [55]="k-L-T-0",[36]="k-R-T-0",[58]="k-L-T-2",[49]="k-L-T-3",
  [51]="k-R-T-1",[117]="k-R-T-2",
  -- L1-only keys → their physical L1 positions
  [33]="k-R-3-4",[30]="k-R-3-5",[42]="k-R-3-6",[24]="k-R-3-3",
  -- L2: F-keys → ZMK physical positions (home row)
  [122]="k-L-2-0",[120]="k-L-2-1",[99]="k-L-2-2",[118]="k-L-2-3",[96]="k-L-2-4",[97]="k-L-2-5",
  [98]="k-R-2-0", [100]="k-R-2-1",[101]="k-R-2-2",[109]="k-R-2-3",[103]="k-R-2-4",[111]="k-R-2-5",
  -- L3: nav keys → ZMK physical positions (YUIO row + HJKL row)
  [115]="k-R-1-0",[121]="k-R-1-1",[116]="k-R-1-2",[119]="k-R-1-3",
  [123]="k-R-2-0",[125]="k-R-2-1",[126]="k-R-2-2",[124]="k-R-2-3",
}

-- Key sets for layer inference
local FKEYS = {[122]=true,[120]=true,[99]=true,[118]=true,[96]=true,[97]=true,
               [98]=true,[100]=true,[101]=true,[109]=true,[103]=true,[111]=true}
local NAVKEYS = {[123]=true,[124]=true,[125]=true,[126]=true,
                 [115]=true,[119]=true,[116]=true,[121]=true}
local L1KEYS  = {[33]=true,[30]=true,[42]=true,[24]=true}

-- ─── Layer inference ──────────────────────────────────────────────────────────
M.currentLayer = 0

local function inferLayer(kc)
  if FKEYS[kc]  then return 2 end
  if NAVKEYS[kc] then return 3 end
  if L1KEYS[kc]  then return 1 end
  return 0
end

local function resetLayerAfterTimeout()
  if M.layerTimer then M.layerTimer:stop() end
  M.layerTimer = hs.timer.doAfter(2.0, function()
    M.currentLayer = 0
    M.layerTimer = nil
    if M.wv then
      M.wv:evaluateJavaScript("hud.setLayer(0)", function() end)
    end
  end)
end

-- ─── Debounced JS update ──────────────────────────────────────────────────────
local function scheduleUpdate(keyID, layer, mode)
  if M.pending then M.pending:stop() end
  M.pending = hs.timer.doAfter(0.016, function()
    M.pending = nil
    if M.wv then
      M.wv:evaluateJavaScript(
        string.format("hud.update('%s',%d,'%s')", keyID, layer, mode),
        function() end
      )
    end
  end)
end

-- ─── WebView setup ────────────────────────────────────────────────────────────
local function defaultFrame()
  local sf = hs.screen.mainScreen():frame()
  return {x=sf.x+sf.w-HUD_W-20, y=sf.y+sf.h-HUD_H-20, w=HUD_W, h=HUD_H}
end

local function getFrame()
  local s = getPos()
  if s then
    local sf = hs.screen.mainScreen():frame()
    if s.x >= sf.x and s.x+HUD_W <= sf.x+sf.w
    and s.y >= sf.y and s.y+HUD_H <= sf.y+sf.h then
      return {x=s.x, y=s.y, w=HUD_W, h=HUD_H}
    end
  end
  return defaultFrame()
end

local htmlPath = hs.configdir .. "/lily58-hud.html"

local function createWebView()
  local wv = hs.webview.new(getFrame(), {developerExtrasEnabled=false})
  wv:windowStyle(hs.webview.windowMasks.borderless)
  wv:level(hs.canvas.windowLevels.floating)
  wv:alpha(0.93)
  wv:allowTextEntry(false)
  wv:transparent(true)
  wv:url("file://" .. htmlPath)

  wv:navigationCallback(function(action, _, _, navItem)
    if action ~= "navigationStarted" then return true end
    local url = navItem and navItem:URL() or ""
    if url:match("^lily58hud://drag") then
      local dx = tonumber(url:match("dx=(-?%d+)"))
      local dy = tonumber(url:match("dy=(-?%d+)"))
      local ox = tonumber(url:match("ox=(-?%d+)"))
      local oy = tonumber(url:match("oy=(-?%d+)"))
      if dx and dy and ox and oy then
        local nx, ny = ox+dx, oy+dy
        wv:topLeft({x=nx, y=ny}); setPos(nx, ny)
      end
      return false
    elseif url:match("^lily58hud://mode") then
      local m = url:match("mode=(%w+)")
      if m then setMode(m) end
      return false
    end
    return true
  end)

  wv:show()
  return wv
end

M.wv = createWebView()

-- ─── Key event watcher ────────────────────────────────────────────────────────
M.tap = hs.eventtap.new({hs.eventtap.event.types.keyDown}, function(event)
  local kc    = event:getKeyCode()
  local keyID = KEY_POS[kc]
  if not keyID then return false end

  local layer = inferLayer(kc)
  if layer ~= 0 then
    M.currentLayer = layer
    resetLayerAfterTimeout()
  elseif M.currentLayer ~= 0 and not M.layerTimer then
    M.currentLayer = 0
  end

  scheduleUpdate(keyID, M.currentLayer, getMode())
  return false
end)

M.tap:start()

-- ─── Modifier state watcher ───────────────────────────────────────────────────
M.modTap = hs.eventtap.new({hs.eventtap.event.types.flagsChanged}, function(event)
  local f = event:getFlags()
  if M.wv then
    M.wv:evaluateJavaScript(
      string.format("hud.setMods(%d,%d,%d,%d)",
        f.cmd   and 1 or 0,
        f.alt   and 1 or 0,
        f.ctrl  and 1 or 0,
        f.shift and 1 or 0),
      function() end
    )
  end
  return false
end)
M.modTap:start()

print("[lily58-hud] loaded — press any key to activate")
