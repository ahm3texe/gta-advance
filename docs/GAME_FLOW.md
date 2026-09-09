# Game flow (observation)

Source: the user's playthrough account dated 2026-09-05 (Europe ROM).
This information CANNOT BE DERIVED from the ROM bytes; it is used as a basis for
naming functions and for mapping the mode/state machine.

## Startup sequence

1. **Language screen** — English among the options. (The Europe version is
   multilingual; the choice is probably written to a persistent settings byte.)
2. **Rockstar logo**, followed by a few presentation screens.
3. **Press Start screen**
   - Menu artwork in the background
   - The "GTA Advance" logo sprite in the center
   - Rockstar Games on the left, Digital Eclipse on the right
   - A blinking "PRESS START" in the center -> blinking driven by a TIMER
   - Music playing
4. **X** -> main menu: `NEW GAME` / `LOAD GAME` / `ERASE GAME`
   - The sprites slide in (transition animation)
   - Characters in the background
5. **NEW GAME** -> **3 save slots**. The first slot was selected.
   - Name entry: "aaa"
   - 3 slots, possibly related to `gSaveSlotHeaders` and `gSaveBuffer`
6. The **"Jump Start" chapter** begins, and **the music changes**.
7. **Character/dialogue screen**
   - Vinnie's portrait on the left, Mike's on the right
   - A text box at the bottom
   - Advanced with **X** (several presses)
8. **The game starts**
   - NPCs walking, car horns
   - The character can walk
9. After walking for a while, a **cutscene screen**
10. Objective text at the bottom: **"ENTER THE CAR"**

## Frame correlation (from the trace log, VERIFIED)

The session recorded with tools/trace.lua matches the account above exactly.
Analysis: `python3 tools/analyze_trace.py`

| Frame | Event | Symbols changed |
|------|------|-------------------|
| f4-f5 | Startup, interrupt setup | gIrqVector, gIrqStack, gSystemStack, gIrqHandlerTable, gIntrMainEwram, gActiveIrqSlot (each ONCE) |
| f4 | **Language screen** | gActiveMenuItemCount 0->**5** (five language options), gActiveMenuItems, gMenuPositionX |
| f19-f20 | Text drawing setup | gFontIndex, gGlyphWidths, gTextVramBase, gTextRowStride |
| f52+ | Frame counter running | gGameState[0] +1 every frame |
| f684 | **Save check** | gCartFlag 0->1, gAddrTable, gRam02027310 |
| f936-f940 | **Main menu** (NEW/LOAD/ERASE) | gFontIndex 192->128, gMenuPaletteSource (palette buffer) |
| **f1066-f1069** | **NEW GAME + slot 1** | gCartFlag 1->0, **player_health 0->100**, **gSessionPtr 0->0x02000F10**, gSlotIds 0->42, gJobTable, gNodeListHead, player_health_copy |
| f1099-f1101 | Chapter loading | gCartFlag 0->1, gRam02000F10, gFrameCounterEwram, gRam02011030, gUnk0202F310 |
| **f1319** | **Dialogue box** (Vinnie/Mike) | gHalfLineSpacing 0->1, gFontIndex 160->240 |
| f1384-f1402 | Advancing the dialogue | gFontIndex 240->242->240, with the **[A]** key |
| **f1507-f1510** | **The game world spawns** | gNodePool, gListHead02016280, gUnk02028270, gFrameDelay -- the linked list system comes into play |
| f1783-f1801 | **Walking** | gDistanceAccum, then mission_timer increasing by 0x100 at a time, with the **[Up]** key |
| f2273-f2754 | **Cutscene** | mission_timer did NOT increase for 481 frames |
| f2303 / f2427 | Dialogue closed / opened | gHalfLineSpacing 1->0 / 0->1 |

## Firm conclusions from this log

- The name and meaning of **player_health** are VERIFIED: it becomes 100 on a new
  game.
- **gSessionPtr** really does point at **gRam02000F10** -- proving that two
  separate symbols refer to the same structure.
- **gActiveMenuItemCount** is 5 at startup; the language screen's five options.
- **gNodePool + gListHead02016280** become active together when the game world
  spawns (f1508): the linked list system is for entity management.
- **gMenuPaletteSource** is NOT A POINTER: its values are BGR555 colors (0x7C1F,
  0x7FFF). It is a palette buffer and must not be read as a u32 address.
- The name **mission_timer** is CORRECT (proven in the fourth session). It
  increased by exactly 0x100 thirty times without the player pressing any key,
  about once every 64 frames. The earlier "distance counter" claim is REFUTED:
  because Up was held continuously, the increments appeared simultaneous with the
  key press — coincidence, not causation.

## Initial inferences (NOT VERIFIED)

- There are at least three distinct "modes": startup/menu, dialogue, and free
  roaming. These may correspond to our `gLoopState` / `gOuterState` /
  `gDisplayState` symbols -- A TRACE LOG IS NEEDED FOR THE MAPPING.
- The blinking "PRESS START" and the NPC animations are tied to the frame
  counter; `gFrameCounterLate` / `gIwramFrameCounter` are the candidates.
- Menu transitions (sliding sprites) require palette and OAM updates;
  `FlushPaletteQueue` (0x08013900) is in that class.
- Three save slots -> `gSaveSlotHeaders` (0x02000460) is probably the array of
  slot headers.

## Next step

Once the trace script (tools/trace.lua) is working, the same flow will be played
again and the RAM symbol changing at each step will be recorded. This document
will then be updated to "observation + address".


## Vehicle run (second session)

The user got into a car and drove backward, forward, right, and left.
Beforehand, they also moved right/left/backward on foot. The difference against
the walking run was taken with `tools/analyze_trace.py`.

**4 symbols that changed only in the vehicle run** (there is NO symbol that
changed in the walking run but not in the vehicle run -- the vehicle run is
additive over walking):

| Symbol | Changes | First frame |
|--------|---------|----------|
| `gFocusPoint`  | 31 | f1345 |
| `gClipBounds`  | 31 | f1345 |
| `gRam020302E0` | 2  | f3815 |
| `gUnk02028290` | 1  | f2864 |

### ~~Firm finding: the focus point is mirrored into IWRAM~~ (RETRACTED -- see below)

`gFocusPoint` (0x020004B0, EWRAM) and `gClipBounds` (0x03000014, IWRAM) are
separate addresses -- 16 MB apart -- yet they carried the same value in ALL 31
transitions, on the same frames. Not a coincidence: the focus point is written to
a working copy in IWRAM.

This connects two functions we are already working on:
- `UpdateFocusPoint` (0x0800A9E4) -- PARKED (4/88)
- `ClipBounds` -- MATCHED (src/core/clip_bounds.c)

The values increase in fixed steps by direction key and wrap at 256:
Up +156, Right +104, Left +152. This is the LOW BYTE of a multi-byte camera
coordinate; the full value requires extended tracing.

### The tool's own blind spot

This run exposed a serious limitation in the tracing tool: sizes other than 1/2/4
were being silently truncated to 1 byte. Only 37 bytes were traced out of the
19086 bytes across 37 symbols. The generator now expands structures word by word
(275 entries) and SAMPLES large arrays, reporting the 18382 untraced bytes.


## Third run: full-width focus point

The tracer now expands structures word by word (275 entries), so both the 8 bytes
of `gFocusPoint` and the 12 bytes of `gClipBounds` are fully visible.

### CORRECTION: the "mirroring" conclusion was wrong

In the second run we said "gFocusPoint and gClipBounds hold the same value 31/31
times." That conclusion was drawn from BYTE 0 ONLY. At full width the picture is
different:

| Comparison | Shared frames | Same value |
|---|---|---|
| `gFocusPoint+0x00` vs `gClipBounds+0x00` | 6  | 5 |
| `gFocusPoint+0x04` vs `gClipBounds+0x04` | 15 | **0** |

The Y component NEVER agrees. The two structures are NOT copies; their roles are
close but their values are separate. Most of the time they also change on
different frames.

This shows why extending the tracer was necessary: the narrow read had
manufactured our own conclusion.

### PROVEN: 16.16 fixed point

The format is not a guess; it was measured:

- The initial values are WHOLE numbers: `gFocusPoint` = (3360.000, 9568.000),
  `gClipBounds` = (3360.000, 9568.000, 164.000)
- Some differences are EXACTLY 65536 (= 1.000) and EXACTLY 262144 (= 4.000)
- With Up held, `gFocusPoint+0x04` steps by EXACTLY -374632 (-5.716) every time:
  **29/29 constant** -> movement at a constant speed

The non-round step of -5.716 may be a speed component that depends on the heading
angle (vehicle orientation), but this is NOT VERIFIED.

### gClipBounds+0x08: a range that opens with speed

The third component, which has no counterpart in the focus point:

| Frame | Value | Key |
|------|-------|-----|
| f688  | 164.0 | (initial value) |
| f3457 | 167.2 | A+Left |
| f3471 | 189.6 | A+Left |
| f3492 | **220.0** | A |
| f3916+ | back toward 164.0 | (throttle released) |

It grows from 164.0 to 220.0 while the throttle is held and returns when it is
released. It looks like a camera range / look-ahead distance that opens with
speed.

---

# Sessions 4-8: targeted tests

After run three, tracing was used in a TARGETED way to close specific questions.
Each session tested a single question; they were recorded separately because the
noise-suppression counter resets per session.

## Session 4 — standing still

**Question:** does `mission_timer` count time or distance?

**Method:** the player pressed NO key for one minute.

**Result:** zero of the 927 events carry a key label; `mission_timer` still
increased 30 times, by exactly 0x100 each time, at intervals of 60-65 frames
(mean 63.9).

**THE NAME IS CORRECT, THE EARLIER CLAIM IS REFUTED.** It had previously been
said that "the increments only happen while Up is held, so it must be a distance
counter." In those sessions Up was held CONTINUOUSLY, so the increments appeared
simultaneous with the key -- coincidence, not causation. The pause during the
cutscene is also the expected behavior for a mission timer.

## Session 5 — damage and death

**Question:** does `player_health` really hold the character's health?

**Result:** it decreased by exactly 4 with each punch, 25 steps from 100 to 0.
`player_health_copy` took the same value ON THE SAME FRAME in 25/25 transitions.

**Death chain (frame by frame):**

| Frame | Event |
|------|------|
| f1794 | health 4->0 **and on the same frame** `gRam02000F10+0x10` was zeroed |
| f1795 | `gNodePool` list heads moved -- an entity was released |
| f1798 | `gRam02011030+0x0C` cleared; an EWRAM pointer written to `gRam02030328` |
| f1882 | `gRam0202F3E0+0x0C` stepped 6->0->7 -- mission cancelled |

`gRam02011030` is the CoordBlock read by the `UpdateFocusPoint` (0x0800A9E4)
function; this establishes which data that parked function operates on.

## Sessions 6-7 — the language screen

**Question:** which of the text table's five slices belongs to which language?

**Result:** as the cursor moved down, `gLanguage` went 0->1->2->3->4, and back as
it moved up. Combined with the menu order: **0=English, 1=Spanish, 2=French,
3=Italian, 4=German**.

**Behavior:** `SetLanguage` is called ON CURSOR MOVEMENT, NOT ON CONFIRMATION --
the menu does a live preview.

Details and verification of the slice mapping: docs/TEXT_MAP.md.

## Session 8 — arrest and the wanted system

The most productive session. Hitting a police car, killing a police officer,
picking up a weapon, being arrested (BUSTED), and mission cancellation all in one
recording.

### Health is actually 16.16 fixed point

`gRam02000F10+0x10` came out consistent across three separate sessions:

- new game: 6553600 = 100 x 65536 = **exactly 100.0**
- arrest: dropped by exactly **262144 (4.0)**, with `player_health` 100->96 on
  the same frame
- death: 4.0 -> 0

So the real value lives here; `player_health` is its integer copy.

### gRam02030330 = the wanted / police system block

| Offset | Observed behavior |
|-------|-------------------|
| +0x08 | state: 0->3->1 when the wanted level starts, 1->2->0 on arrest |
| +0x0C | threshold: 40->25 while wanted, 25->40 when cleared |
| +0x10 | `wanted_level_true` |
| +0x18, +0x1C | two countdowns decreasing together (1800->1770 / 300->270) |
| +0x20 | 0->2400 while wanted, 2400->0 on arrest |
| +0x28, +0x30 | set while wanted, zeroed on arrest |
| +0x34 | 0->1 on arrest |
| +0x38 | BUSTED screen step counter: 0->1->2->3->4->5 |

`wanted_level_display` (0x0202581A) and `wanted_level_true` (0x02030340) take the
same value ON THE SAME FRAME; they are two separate addresses, one being the
display copy. Both names turned out to be CORRECT.

### A recorded error, resolved

`gRam02030330` (size 60) and `wanted_level_true` (0x02030340) had previously been
flagged as "overlapping" with no decision reached. Seeing writes to the block's
+0x18 and +0x1C fields settled it: there is NO overlap, and `wanted_level_true`
is that block's **+0x10 field**. The cheat database had given the address of an
inner field of a larger structure.

## The tool's own defect: double loading

In these sessions the same transitions appeared in the log with TWO different
frame numbers (for example f32645 and f620, a constant 32025 apart). The cause:
the script was reloaded before each test but the previous instances were never
closed, and mGBA kept every instance's frame callback registered.

An INCREMENTING COUNTER guard was added to the generator: a new load increments
`__TRACE_EPOCH`, and the old instance shuts itself down on the first frame. The
first guard written used a BOOLEAN flag and was faulty -- because the second load
wrote the same value, the old instance passed the test and kept running.

## Closed / still open questions

**Closed:** the mission_timer name, the meaning of player_health, health's 16.16
format, the language order and slice mapping, the text encoding (Latin-1), the
gRam02030330 overlap, and the correctness of the wanted_level names.

**Open:** the name `gFontIndex` is still doubtful (the values look more like a
width/position than a font index). The name `gGameState` is unsupported (its byte
0 behaves like a counter). The transparency effect under the bridge could not be
traced -- MMIO (REG_BLDCNT / REG_BLDALPHA) is deliberately not traced, and would
need a separate script.
