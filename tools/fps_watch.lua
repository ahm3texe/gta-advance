-- Frame rate and logic step measurement - GTA Advance
--
-- Loading: mGBA > Tools > Scripting... > File > Load script
-- Output:  the console in the Scripting window (and build/fps_watch.log)
--
-- WHAT IT MEASURES
-- ----------------
-- The FPS in mGBA's title bar is the *emulator's* speed; it does not show the
-- game's logic step. The game uses a variable time step (see
-- src/interrupt/vblank_intr.c, 0x08000220, byte-matching):
--
--   gIwramFrameCounter (0x03000004) increments on every hardware VBlank
--   When a logic frame ends: gFrameDelay = gIwramFrameCounter, clamped at 5
--   then gIwramFrameCounter is zeroed
--
-- So gFrameDelay = "how many hardware frames have passed since the last logic
-- frame". 1 means the game runs every frame (60 fps). 2 means it skips a frame
-- (30 fps). 3+ is worse.
--
-- CAUTION: while gGameState[12] is 1 or 2 (link/two-player mode) the VBlank
-- handler fixes gFrameDelay at 5 REGARDLESS of the frames actually elapsed. In
-- that mode this measurement does not give the logic rate; the raw
-- gIwramFrameCounter distribution is also printed below, and that is the real
-- value.

-- Resolved from this script's own location so the path is not machine-specific.
-- Override with FPS_WATCH_LOG if mGBA is run from elsewhere.
local SCRIPT_DIR = (debug.getinfo(1, "S").source:match("@(.*/)") or "./")
local LOG_PATH = os.getenv("FPS_WATCH_LOG") or (SCRIPT_DIR .. "../build/fps_watch.log")

local ADDR_FRAME_DELAY   = 0x03000000
local ADDR_IWRAM_COUNTER = 0x03000004
local ADDR_GAME_STATE    = 0x02000CE0   -- +12 = player count / mode flag
local REPORT_EVERY       = 60           -- hardware frames

-- Double-load guard: mGBA registers a new callback on every load
-- and the old one keeps running, doubling the numbers.
_G.__FPS_EPOCH = (_G.__FPS_EPOCH or 0) + 1
local MY_EPOCH = _G.__FPS_EPOCH
if MY_EPOCH > 1 then
  console:log(string.format("[info] load %d; the previous measurement was silenced", MY_EPOCH))
end

local log = io.open(LOG_PATH, "a")
local function out(line)
  console:log(line)
  if log then log:write(line .. "\n"); log:flush() end
end

out(string.format("=== measurement started (load %d) ===", MY_EPOCH))

local frames        = 0        -- hardware frames elapsed in this window
local logicFrames   = 0        -- the moment the counter resets = one logic frame
local prevCounter   = -1
local hist          = {}       -- observed step -> occurrence count
local windows       = 0

callbacks:add("frame", function()
  if MY_EPOCH ~= _G.__FPS_EPOCH then return end   -- silence the stale instance

  frames = frames + 1

  -- The moment the counter returns to zero a logic frame has completed; the
  -- value BEFORE the reset is how many hardware frames that logic frame took.
  local counter = emu:read32(ADDR_IWRAM_COUNTER)
  if prevCounter >= 0 and counter < prevCounter then
    logicFrames = logicFrames + 1
    local step = prevCounter
    hist[step] = (hist[step] or 0) + 1
  end
  prevCounter = counter

  if frames >= REPORT_EVERY then
    windows = windows + 1
    local delay = emu:read32(ADDR_FRAME_DELAY)
    local mode  = emu:read8(ADDR_GAME_STATE + 12)

    -- Format the distribution for readability
    local parts, keys = {}, {}
    for k in pairs(hist) do keys[#keys + 1] = k end
    table.sort(keys)
    for _, k in ipairs(keys) do
      parts[#parts + 1] = string.format("%d:%d", k, hist[k])
    end

    -- Logic fps = logic frames completed in 60 hardware frames
    out(string.format(
      "window %-3d  logic %2d/60 frames  (~%2d fps)  gFrameDelay=%d  mode=%d  step distribution [%s]",
      windows, logicFrames, logicFrames, delay, mode,
      table.concat(parts, " ")))

    if mode == 1 or mode == 2 then
      out("             WARNING: gFrameDelay is FIXED at 5 in this mode; " ..
          "the real step can only be read from the distribution.")
    end

    frames, logicFrames, hist = 0, 0, {}
  end
end)

out("Play the game; one line is printed every second.")
out("A step distribution of 1:60 means it runs every frame (60 fps).")
out("2:30 means every other frame (30 fps) - that is the 'stutter' feel.")
