#!/usr/bin/env python3
"""Generate an mGBA Lua tracing script from ram_map.csv.

mGBA 0.10.5's Lua API has NO breakpoints (setBreakpoint); what we have is
read8/16/32 and a frame callback. Hence the approach: scan the known RAM symbols
every frame and log the ONES THAT CHANGED, together with the key state at that
moment.

Usage:
    python3 tools/make_trace_script.py
    -> tools/trace.lua

Then in mGBA:   Tools > Scripting... > Load script > tools/trace.lua
Log file:    build/trace.log
"""
import csv
import pathlib

ROOT = pathlib.Path(__file__).resolve().parent.parent
RAM_MAP = ROOT / "data" / "ram_map.csv"
OUT = ROOT / "tools" / "trace.lua"
LOG = ROOT / "build" / "trace.log"

# EWRAM (0x02......) and IWRAM (0x03......) only. MMIO registers (0x04......)
# change every frame and flood the log; ROM addresses are constant anyway.
WATCH_PREFIXES = (0x02, 0x03)

# Structures up to this size are traced COMPLETELY; larger ones are sampled
# and the untraced part is reported.
STRUCT_FULL_LIMIT = 64
SAMPLE_WORDS = 4


LUA_TEMPLATE = r"""-- AUTOMATICALLY GENERATED: tools/make_trace_script.py
-- Do not edit by hand; update ram_map.csv and re-run the generator.
--
-- RAM change tracer for mGBA 0.10.5.
-- Load:    Tools > Scripting... > Load script
-- Log:     __LOG_PATH__

-- DOUBLE-LOAD GUARD.  If the script is loaded more than once, mGBA
-- keeps each frame callback SEPARATELY REGISTERED, so every change is
-- logged more than once with different frame counters (this happened to us:
-- the same transition appeared twice, as f32645 and f620). Here we
-- neutralise the previous instance.
-- A boolean flag is NOT ENOUGH: the second load writes the same value, so the
-- old instance passes the "is it different" test and keeps running. What is
-- needed is a counter that INCREASES on every load.
_G.__TRACE_EPOCH = (_G.__TRACE_EPOCH or 0) + 1
local MY_EPOCH = _G.__TRACE_EPOCH
if MY_EPOCH > 1 then
  console:log(string.format("[info] load %d; previous instance(s) silenced", MY_EPOCH))
end

local LOG_PATH = "__LOG_PATH__"
local WATCH = {
__ENTRIES__
}

local prev    = {}
local nchg    = {}   -- number of changes per symbol
local noisy   = {}   -- the ones suppressed for being noisy
local frame   = 0
local fh      = nil
local keys_ok = true

-- Counters/RNG that change every frame flood the log. Once a symbol exceeds
-- this limit it is suppressed and reported once.
local NOISE_LIMIT = 30

local KEY_NAMES = {
  [0]="A", [1]="B", [2]="Select", [3]="Start",
  [4]="Right", [5]="Left", [6]="Up", [7]="Down", [8]="R", [9]="L",
}

local function out(line)
  console:log(line)
  if fh then fh:write(line .. "\n"); fh:flush() end
end

local function readN(addr, size)
  if size == 4 then return emu:read32(addr)
  elseif size == 2 then return emu:read16(addr)
  else return emu:read8(addr) end
end

-- Reading the key state differs between versions; on failure
-- keep tracing without keys rather than crashing.
local function keyString()
  if not keys_ok then return "" end
  local ok, mask = pcall(function() return emu:getKeys() end)
  if not ok or type(mask) ~= "number" then
    keys_ok = false
    out("[warning] emu:getKeys() is unavailable; the key column is disabled")
    return ""
  end
  local held = {}
  for bit = 0, 9 do
    if mask & (1 << bit) ~= 0 then held[#held + 1] = KEY_NAMES[bit] end
  end
  if #held == 0 then return "" end
  return " [" .. table.concat(held, "+") .. "]"
end

-- Validation happens ON THE FIRST FRAME, NOT while the script loads: the
-- `emu` object is not ready yet at load time, and validating at startup
-- emptied the whole list and ended the session with zero data.
local validated = false

-- An "all failed" symptom has two possible causes: `emu` is not ready yet,
-- OR the method names differ in this version. The two look the same, so we
-- probe the API form first and print the ERROR TEXT; that way a single
-- session tells you exactly which one it is.
local function probeApi()
  local probe = 0x02000000
  local attempts = {
    {"emu:read32(addr)",        function() return emu:read32(probe) end},
    {"emu:read8(addr)",         function() return emu:read8(probe) end},
    {"emu.memory.wram:read32()",function() return emu.memory.wram:read32(probe) end},
    {"emu:readRange(addr,4)",   function() return emu:readRange(probe, 4) end},
  }
  out("--- API probe ---")
  local winner = nil
  for i = 1, #attempts do
    local name, fn = attempts[i][1], attempts[i][2]
    local ok, res = pcall(fn)
    if ok then
      out(string.format("  WORKED   %-26s -> %s", name, tostring(res)))
      if winner == nil then winner = name end
    else
      out(string.format("  error    %-26s -> %s", name, tostring(res)))
    end
  end
  if winner == nil then
    out("[ERROR] no read form worked. Report the error texts above;")
    out("        they identify the right API form for this mGBA version.")
  else
    out("selected API form: " .. winner)
  end
  out("--- end of probe ---")
  return winner ~= nil
end

local function validateOnce()
  validated = true
  if not probeApi() then return end
  local bad, first_err = 0, nil
  for i = #WATCH, 1, -1 do
    local ok, err = pcall(readN, WATCH[i][1], WATCH[i][2])
    if not ok then
      if first_err == nil then
        first_err = tostring(err)
        out("[warning] first read error: " .. first_err)
      end
      table.remove(WATCH, i)
      bad = bad + 1
    end
  end
  out(string.format("tracing active: %d symbols (%d skipped), noise threshold %d",
        #WATCH, bad, NOISE_LIMIT))
end

local function onFrame()
  -- If this instance is stale (a new load happened) do nothing.
  if MY_EPOCH ~= _G.__TRACE_EPOCH then return end
  if not validated then validateOnce() end
  frame = frame + 1
  local keys = nil
  for i = 1, #WATCH do
    if not noisy[i] then
      local e = WATCH[i]
      -- NO pcall: the addresses were validated once at startup, and in the hot loop
      -- 135 pcall invocations per frame unnecessarily slowed down the emulator.
      local val = readN(e[1], e[2])
      local old = prev[i]
      if old ~= nil and old ~= val then
        local c = (nchg[i] or 0) + 1
        nchg[i] = c
        if c > NOISE_LIMIT then
          noisy[i] = true
          out(string.format("f%-7d %-24s ... SUPPRESSED (%d changes; may be a counter/RNG)",
                frame, e[3], c))
        else
          if keys == nil then keys = keyString() end
          out(string.format("f%-7d %-24s %s -> %s%s",
                frame, e[3], tostring(old), tostring(val), keys))
        end
      end
      prev[i] = val
    end
  end
end

fh = io.open(LOG_PATH, "a")
if fh then
  fh:write("\n==== new session ====\n")
  fh:flush()
else
  console:log("[warning] could not open log file: " .. LOG_PATH)
end

out(string.format("script loaded: %d symbols; validation on the first frame", #WATCH))
callbacks:add("frame", onFrame)
"""


def validate_lua(text: str):
    """Return the lines that contain an unterminated string literal.

    In Lua a quoted string literal cannot span lines. An escaping bug once
    produced this invalid form, so generation stops if this check finds it.
    """
    bad = []
    for i, line in enumerate(text.splitlines(), 1):
        depth, esc, quotes = 0, False, 0
        for ch in line:
            if esc:
                esc = False
            elif ch == "\\":
                esc = True
            elif ch == '"':
                quotes += 1
        if quotes % 2:
            bad.append((i, line))
    return bad


def main() -> int:
    rows = list(csv.DictReader(RAM_MAP.open()))
    watched, skipped, sampled = [], [], []
    for r in rows:
        addr = int(r["address"], 16)
        if (addr >> 24) not in WATCH_PREFIXES:
            skipped.append(r["name"])
            continue
        try:
            size = int(r["size"] or 1)
        except ValueError:
            size = 1
        if size in (1, 2, 4):
            watched.append((addr, size, r["name"]))
            continue
        # A multi-byte structure/array. It used to be silently truncated to
        # 1 byte, leaving 19049 of 37 symbols' 19086 bytes BLIND. Now it is
        # expanded word by word; large arrays are SAMPLED and the untraced part
        # is reported (reading 19 KB every frame chokes the emulator).
        words = (size + 3) // 4
        take = words if size <= STRUCT_FULL_LIMIT else SAMPLE_WORDS
        for w in range(take):
            watched.append((addr + w * 4, 4, f'{r["name"]}+0x{w * 4:02X}'))
        if take < words:
            sampled.append((r["name"], size, take * 4))

    watched.sort()
    entries = ",\n".join(
        f'  {{0x{a:08X}, {s}, "{n}"}}' for a, s, n in watched
    )

    # The template is a RAW string with placeholders: with an f-string, Python
    # would turn Lua's \n escapes into real line breaks and split the string
    # literals in two (this bit us once).
    lua = LUA_TEMPLATE.replace("__LOG_PATH__", str(LOG))\
                      .replace("__ENTRIES__", entries)

    problems = validate_lua(lua)
    if problems:
        for line_no, text in problems:
            print(f"ERROR: line {line_no}: unterminated string -> {text.strip()[:60]}")
        return 1

    OUT.write_text(lua)

    print(f"written: {OUT.relative_to(ROOT)}")
    print(f"  traced: {len(watched)} symbols")
    print(f"  skipped: {len(skipped)} (MMIO/ROM) -> {', '.join(skipped)}")
    if sampled:
        blind = sum(size - seen for _, size, seen in sampled)
        print(f"  SAMPLED: {len(sampled)} large arrays, {blind} bytes not traced")
        for name, size, seen in sorted(sampled, key=lambda x: -x[1])[:6]:
            print(f"    {name:<22} {size:>6} bytes, first {seen} traced")
    print(f"  log:     {LOG.relative_to(ROOT)}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
