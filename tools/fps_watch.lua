-- Kare hizi ve mantik adimi olcumu — GTA Advance
--
-- Yukleme: mGBA > Tools > Scripting... > File > Load script
-- Cikti:   Scripting penceresindeki konsol (ve build/fps_watch.log)
--
-- NE OLCUYOR
-- ----------
-- mGBA'nin baslik cubugundaki FPS *emulatorun* hizidir; oyunun mantik
-- adimini gostermez. Oyun degisken zaman adimi kullaniyor (bkz.
-- src/interrupt/vblank_intr.c, 0x08000220 byte-matching):
--
--   gIwramFrameCounter (0x03000004) her donanim VBlank'inde artiyor
--   Mantik karesi bitince: gFrameDelay = gIwramFrameCounter, 5'te kirpiliyor
--   sonra gIwramFrameCounter sifirlaniyor
--
-- Yani gFrameDelay = "son mantik karesinden bu yana kac donanim karesi
-- gecti". 1 ise oyun her karede calisiyor (60 fps). 2 ise bir kare
-- atliyor (30 fps). 3+ ise daha kotu.
--
-- DIKKAT: gGameState[12] degeri 1 ya da 2 iken (baglanti/iki oyunculu
-- kip) VBlank isleyicisi gFrameDelay'i gercek gecen kareye BAKMAKSIZIN
-- 5'e sabitliyor. O kipte bu olcum mantik hizini vermez; asagida ayrica
-- ham gIwramFrameCounter dagilimi da basiliyor, gercek deger odur.

local LOG_PATH = "/Users/muhammetyildirim/Documents/ChatGPT/advance-decomp/build/fps_watch.log"

local ADDR_FRAME_DELAY   = 0x03000000
local ADDR_IWRAM_COUNTER = 0x03000004
local ADDR_GAME_STATE    = 0x02000CE0   -- +12 = oyuncu sayisi / kip bayragi
local REPORT_EVERY       = 60           -- donanim karesi

-- Cift yukleme korumasi: mGBA her yuklemede yeni bir geri cagri kaydeder
-- ve eskisi calismaya devam eder, sayilar ikiye katlanir.
_G.__FPS_EPOCH = (_G.__FPS_EPOCH or 0) + 1
local MY_EPOCH = _G.__FPS_EPOCH
if MY_EPOCH > 1 then
  console:log(string.format("[bilgi] %d. yukleme; onceki olcum susturuldu", MY_EPOCH))
end

local log = io.open(LOG_PATH, "a")
local function out(line)
  console:log(line)
  if log then log:write(line .. "\n"); log:flush() end
end

out(string.format("=== olcum basladi (yukleme %d) ===", MY_EPOCH))

local frames        = 0        -- bu pencerede gecen donanim karesi
local logicFrames   = 0        -- sayacin sifirlandigi an = bir mantik karesi
local prevCounter   = -1
local hist          = {}       -- gozlenen adim -> kac kez
local windows       = 0

callbacks:add("frame", function()
  if MY_EPOCH ~= _G.__FPS_EPOCH then return end   -- eski ornek sussun

  frames = frames + 1

  -- Sayac sifira dondugu an bir mantik karesi tamamlanmistir; sifirlanmadan
  -- ONCEKI deger, o mantik karesinin kac donanim karesi surdugudur.
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

    -- Dagilimi okunur bicimde topla
    local parts, keys = {}, {}
    for k in pairs(hist) do keys[#keys + 1] = k end
    table.sort(keys)
    for _, k in ipairs(keys) do
      parts[#parts + 1] = string.format("%d:%d", k, hist[k])
    end

    -- Mantik fps'i = 60 donanim karesinde kac mantik karesi tamamlandi
    out(string.format(
      "pencere %-3d  mantik %2d/60 kare  (~%2d fps)  gFrameDelay=%d  kip=%d  adim dagilimi[%s]",
      windows, logicFrames, logicFrames, delay, mode,
      table.concat(parts, " ")))

    if mode == 1 or mode == 2 then
      out("             UYARI: bu kipte gFrameDelay 5'e SABITLENIYOR; " ..
          "gercek adim yalnizca dagilimdan okunur")
    end

    frames, logicFrames, hist = 0, 0, {}
  end
end)

out("Oyunu oynat; her saniye bir satir dusecek.")
out("adim dagilimi 1:60 ise her karede calisiyor (60 fps).")
out("2:30 ise her iki karede bir calisiyor (30 fps) — 'atliyor' hissi budur.")
