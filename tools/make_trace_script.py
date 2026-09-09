#!/usr/bin/env python3
"""Generate an mGBA Lua tracing script from ram_map.csv.

mGBA 0.10.5's Lua API has NO breakpoints (setBreakpoint); what we have is
read8/16/32 and a frame callback. Hence the approach: scan the known RAM symbols
every frame and log the ONES THAT CHANGED, together with the key state at that
moment.

Usage:
    python3 tools/make_trace_script.py
    -> tools/trace.lua

Sonra mGBA'da:  Tools > Scripting... > Load script > tools/trace.lua
Log dosyasi:    build/trace.log
"""
import csv
import pathlib

ROOT = pathlib.Path(__file__).resolve().parent.parent
RAM_MAP = ROOT / "data" / "ram_map.csv"
OUT = ROOT / "tools" / "trace.lua"
LOG = ROOT / "build" / "trace.log"

# Sadece EWRAM (0x02......) ve IWRAM (0x03......).  MMIO (0x04......) her
# karede degisiyor, logu bogar; ROM adresleri zaten sabit.
WATCH_PREFIXES = (0x02, 0x03)

# Bu boyuta kadar olan yapilar TAMAMEN izleniyor; ustu orneklenip
# izlenmeyen kismi raporlaniyor.
STRUCT_FULL_LIMIT = 64
SAMPLE_WORDS = 4


LUA_TEMPLATE = r"""-- OTOMATIK URETILDI: tools/make_trace_script.py
-- Elle duzenleme; ram_map.csv'yi guncelleyip ureteci tekrar calistir.
--
-- RAM change tracer for mGBA 0.10.5.
-- Yukleme: Tools > Scripting... > Load script
-- Log:     __LOG_PATH__

-- CIFT YUKLEME KORUMASI.  Script birden fazla kez yuklenirse mGBA her
-- ornegin kare geri cagrisini AYRI KAYITLI tutuyor ve her degisiklik
-- logged more than once with different frame counters (this happened to us:
-- the same transition appeared twice, as f32645 and f620). The previous
-- ornegi burada etkisizlestiriyoruz.
-- A boolean flag is NOT ENOUGH: the second load writes the same value, so the
-- old instance passes the "is it different" test and keeps running. Each
-- yuklemede ARTAN bir sayac gerekiyor.
_G.__TRACE_EPOCH = (_G.__TRACE_EPOCH or 0) + 1
local MY_EPOCH = _G.__TRACE_EPOCH
if MY_EPOCH > 1 then
  console:log(string.format("[bilgi] %d. yukleme; onceki ornek(ler) susturuldu", MY_EPOCH))
end

local LOG_PATH = "__LOG_PATH__"
local WATCH = {
__ENTRIES__
}

local prev    = {}
local nchg    = {}   -- number of changes per symbol
local noisy   = {}   -- gurultulu diye susturulanlar
local frame   = 0
local fh      = nil
local keys_ok = true

-- Counters/RNG that change every frame flood the log. Once a symbol exceeds
-- gecerse susturulup bir kez rapor ediliyor.
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

-- Tus durumunu okumak surumden surume degisiyor; basarisiz olursa
-- izlemeye tussuz devam et, cokme.
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

-- Validation happens ON THE FIRST FRAME, NOT while the script loads: `emu`
-- nesnesi yukleme aninda henuz hazir olmuyor ve acilista dogrulamak
-- tum listeyi bosaltip oturumu sifir veriyle bitiriyordu.
local validated = false

-- "hepsi basarisiz" belirtisi iki ayri sebepten olabilir: `emu` hazir
-- ready, OR the method names differ in this version. The two look the same,
-- so we probe the API form first and print the ERROR TEXT; that way
-- tek bir oturum hangisi oldugunu kesin soyluyor.
local function probeApi()
  local probe = 0x02000000
  local attempts = {
    {"emu:read32(addr)",        function() return emu:read32(probe) end},
    {"emu:read8(addr)",         function() return emu:read8(probe) end},
    {"emu.memory.wram:read32()",function() return emu.memory.wram:read32(probe) end},
    {"emu:readRange(addr,4)",   function() return emu:readRange(probe, 4) end},
  }
  out("--- API yoklamasi ---")
  local winner = nil
  for i = 1, #attempts do
    local name, fn = attempts[i][1], attempts[i][2]
    local ok, res = pcall(fn)
    if ok then
      out(string.format("  CALISTI  %-26s -> %s", name, tostring(res)))
      if winner == nil then winner = name end
    else
      out(string.format("  error    %-26s -> %s", name, tostring(res)))
    end
  end
  if winner == nil then
    out("[ERROR] no read form worked. Report the error texts above")
    out("       Claude'a gonder; dogru API bicimi oradan cikar.")
  else
    out("kullanilan bicim: " .. winner)
  end
  out("--- yoklama sonu ---")
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
        out("[uyari] ilk okuma hatasi: " .. first_err)
      end
      table.remove(WATCH, i)
      bad = bad + 1
    end
  end
  out(string.format("tracing active: %d symbols (%d skipped), noise threshold %d",
        #WATCH, bad, NOISE_LIMIT))
end

local function onFrame()
  -- Bu ornek eskidiyse (yeni bir yukleme oldu) hicbir sey yapma.
  if MY_EPOCH ~= _G.__TRACE_EPOCH then return end
  if not validated then validateOnce() end
  frame = frame + 1
  local keys = nil
  for i = 1, #WATCH do
    if not noisy[i] then
      local e = WATCH[i]
      -- NO pcall: the addresses were validated once at startup, and in the hot loop
      -- kare basina 135 pcall emulatoru gereksiz yavaslatiyordu.
      local val = readN(e[1], e[2])
      local old = prev[i]
      if old ~= nil and old ~= val then
        local c = (nchg[i] or 0) + 1
        nchg[i] = c
        if c > NOISE_LIMIT then
          noisy[i] = true
          out(string.format("f%-7d %-24s ... SUSTURULDU (%d degisim; sayac/RNG olabilir)",
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
  fh:write("\n==== yeni oturum ====\n")
  fh:flush()
else
  console:log("[uyari] log dosyasi acilamadi: " .. LOG_PATH)
end

out(string.format("script loaded: %d symbols; validation on the first frame", #WATCH))
callbacks:add("frame", onFrame)
"""


def validate_lua(text: str):
    """Kapanmamis string literali olan satirlari dondur.

    Lua'da string literalleri satir sonunu gecemez.  Kacis hatasi tam
    produced this form, so generation builds on it.
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
        # Cok baytli yapi/dizi.  Eskiden sessizce 1 bayta kirpiliyordu ve
        # 37 sembolun 19086 baytinin 19049'u KOR kaliyordu.  Simdi kelime
        # kelime aciliyor; buyuk diziler ORNEKLENIYOR ve izlenmeyen kisim
        # raporlaniyor (her kareyi 19 KB okumak emulatoru boguyor).
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

    # Template HAM (raw) string ve yer tutuculu: f-string kullanilirsa
    # Lua'nin \n kacislari Python tarafindan gercek satir sonuna cevrilip
    # string literalleri ikiye boluyor (bir kez basimiza geldi).
    lua = LUA_TEMPLATE.replace("__LOG_PATH__", str(LOG))\
                      .replace("__ENTRIES__", entries)

    problems = validate_lua(lua)
    if problems:
        for line_no, text in problems:
            print(f"ERROR: line {line_no}: unterminated string -> {text.strip()[:60]}")
        return 1

    OUT.write_text(lua)

    print(f"yazildi: {OUT.relative_to(ROOT)}")
    print(f"  traced: {len(watched)} symbols")
    print(f"  atlanan: {len(skipped)} (MMIO/ROM) -> {', '.join(skipped)}")
    if sampled:
        blind = sum(size - seen for _, size, seen in sampled)
        print(f"  SAMPLED: {len(sampled)} large arrays, {blind} bytes not traced")
        for name, size, seen in sorted(sampled, key=lambda x: -x[1])[:6]:
            print(f"    {name:<22} {size:>6} bayttan ilk {seen}")
    print(f"  log:     {LOG.relative_to(ROOT)}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
