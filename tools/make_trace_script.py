#!/usr/bin/env python3
"""ram_map.csv'den mGBA Lua izleme scripti uretir.

mGBA 0.10.5'in Lua API'sinde kesme noktasi (setBreakpoint) YOK; elimizde
read8/16/32 ve kare geri cagrisi var.  Bu yuzden yaklasim: her karede
bilinen RAM sembollerini tarayip DEGISENLERI, o andaki tus durumuyla
birlikte loglamak.

Kullanim:
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


LUA_TEMPLATE = r"""-- OTOMATIK URETILDI: tools/make_trace_script.py
-- Elle duzenleme; ram_map.csv'yi guncelleyip ureteci tekrar calistir.
--
-- mGBA 0.10.5 icin RAM degisim izleyicisi.
-- Yukleme: Tools > Scripting... > Load script
-- Log:     __LOG_PATH__

local LOG_PATH = "__LOG_PATH__"
local WATCH = {
__ENTRIES__
}

local prev    = {}
local nchg    = {}   -- sembol basina degisim sayisi
local noisy   = {}   -- gurultulu diye susturulanlar
local frame   = 0
local fh      = nil
local keys_ok = true

-- Her karede degisen sayaclar/RNG logu bogar.  Bir sembol bu esigi
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
    out("[uyari] emu:getKeys() yok; tus sutunu kapatildi")
    return ""
  end
  local held = {}
  for bit = 0, 9 do
    if mask & (1 << bit) ~= 0 then held[#held + 1] = KEY_NAMES[bit] end
  end
  if #held == 0 then return "" end
  return " [" .. table.concat(held, "+") .. "]"
end

-- Dogrulama ILK KAREDE yapiliyor, script yuklenirken DEGIL: `emu`
-- nesnesi yukleme aninda henuz hazir olmuyor ve acilista dogrulamak
-- tum listeyi bosaltip oturumu sifir veriyle bitiriyordu.
local validated = false

-- "hepsi basarisiz" belirtisi iki ayri sebepten olabilir: `emu` hazir
-- degil, YA DA metot adlari bu surumde farkli.  Ikisi ayni gorunuyor,
-- o yuzden once API bicimini yoklayip HATA METNINI yaziyoruz; boylece
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
      out(string.format("  hata     %-26s -> %s", name, tostring(res)))
    end
  end
  if winner == nil then
    out("[HATA] hicbir okuma bicimi calismadi. Yukaridaki hata metinlerini")
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
  out(string.format("izleme aktif: %d sembol (%d atlandi), gurultu esigi %d",
        #WATCH, bad, NOISE_LIMIT))
end

local function onFrame()
  if not validated then validateOnce() end
  frame = frame + 1
  local keys = nil
  for i = 1, #WATCH do
    if not noisy[i] then
      local e = WATCH[i]
      -- pcall YOK: adresler acilista bir kez dogrulandi, sicak dongude
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

out(string.format("script yuklendi: %d sembol; dogrulama ilk karede", #WATCH))
callbacks:add("frame", onFrame)
"""


def validate_lua(text: str):
    """Kapanmamis string literali olan satirlari dondur.

    Lua'da string literalleri satir sonunu gecemez.  Kacis hatasi tam
    olarak bunu uretiyordu, o yuzden uretim bunun uzerinde durur.
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
    watched, skipped = [], []
    for r in rows:
        addr = int(r["address"], 16)
        if (addr >> 24) not in WATCH_PREFIXES:
            skipped.append(r["name"])
            continue
        try:
            size = int(r["size"] or 1)
        except ValueError:
            size = 1
        if size not in (1, 2, 4):
            size = 1          # dizi/yapi: ilk baytini izle
        watched.append((addr, size, r["name"]))

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
            print(f"HATA: satir {line_no}: kapanmamis string -> {text.strip()[:60]}")
        return 1

    OUT.write_text(lua)

    print(f"yazildi: {OUT.relative_to(ROOT)}")
    print(f"  izlenen: {len(watched)} sembol")
    print(f"  atlanan: {len(skipped)} (MMIO/ROM) -> {', '.join(skipped)}")
    print(f"  log:     {LOG.relative_to(ROOT)}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
