# In-ROM traces and external sources

## Standard library: the ROM links against agbcc's newlib

There is a library signature block around `0x00BD3450`:

```
0x0BD3450  MultiSioSync020820
0x0BD3470  ASSERTION FAILED  FILE=[%s] LINE=[%d]  EXP=[%s]
0x0BD34A4  WARING FILE=[%s] LINE=[%d]  EXP=[%s]
0x0BD34D4  EEPROM_V124
0x0BD3644  0123456789abcdef
0x0BD3658  (null)
0x0BD3674  bug in vfprintf: bad base
0x0BD3698  Infinity
0x0BD3810  _sbrk: Heap and stack collision
```

The last three strings are **present verbatim in `tools/agbcc/lib/libc.a`.** So
the ROM's code tail is standard library code that requires no reverse
engineering, and we already have its source.

`MultiSioSync020820` is the version stamp of Nintendo's serial communication
library (20 August 2002). `EEPROM_V124` was already known.

### Verified matches

```sh
make scan-libc
```

This searches the ROM for function bodies from libc.a:

| Address | Function | Size | Note |
|---|---|---|---|
| `0x0806DE84` | `_toupper` | 28 B | masked; ambiguous with `toupper` |
| `0x08070CE4` | `_mbtowc_r` | 42 B | |
| `0x08070DF8` | `_Bfree` | 24 B | |
| `0x08070F34` | `_hi0bits` | 88 B | |
| `0x08070F8C` | `_lo0bits` | 130 B | |
| `0x080716A4` | `isinf` | 36 B | **missed by Ghidra** |
| `0x080716C8` | `isnan` | 32 B | **missed by Ghidra** |
| `0x080717EC` | `findslot` | 30 B | masked |
| `0x0807180C` | `remap_handle` | 76 B | masked |
| `0x08071B9C` | `_exit` | 32 B | **missed by Ghidra**; ambiguous with `_kill` |
| `0x08071D8C` | `abort` | 32 B | |

### Verified libc regions

Functions that match without relocation are now wired into the build:

```sh
make libc-verify
```

For each entry in `data/libc_regions.csv`, the function body is extracted from
`tools/agbcc/lib/libc.a` and compared against the ROM. `make matching` runs this
automatically.

| Address | Function | Size |
|---|---|---|
| `0x08070CE4` | `_mbtowc_r` | 42 B |
| `0x08070DF8` | `_Bfree` | 24 B |
| `0x08070F34` | `_hi0bits` | 88 B |
| `0x08070F8C` | `_lo0bits` | 130 B |
| `0x080716A4` | `isinf` | 36 B |
| `0x080716C8` | `isnan` | 32 B |
| `0x08071B9C` | `_exit` | 32 B |
| `0x08071BBC` | `_kill` | 32 B |
| `0x08071D8C` | `abort` | 32 B |

**448 bytes** in total. These are not a reverse-engineering result: they are
proof that standard library code whose source we already have is byte-identical
to what is in the ROM. Total verified ROM area went from 5340 to **5788 bytes**.

The `_exit` / `_kill` ambiguity is resolved. Their bodies are identical, but this
body occurs **exactly twice** in the ROM, 32 bytes apart — the same layout as in
`syscalls.o` (`_exit` at object offset 892, `_kill` at 924). The pair can only be
laid out in that order, so `_exit = 0x08071B9C` and `_kill = 0x08071BBC` are
certain.

This shows that an ambiguity byte comparison alone cannot settle can be resolved
by a *layout argument*: a usable method for symbol pairs with identical bodies.

### Masked search

Functions containing external calls do not match the ROM bytes until they are
linked: the `bl` target and the addresses in the literal pool depend on the
binding. The scan is therefore **masked** — the bytes touched by relocation are
treated as wildcards and the rest of the body is searched verbatim. This finds a
function without knowing its address.

The method was visually verified on `remap_handle` (`0x0807180C`, 76 B): all 37
instructions are identical, and the only apparent differences are literal pool
words — exactly the bytes that are masked.

Ambiguity is flagged honestly: because the bodies of the `toupper`/`_toupper` and
`_exit`/`_kill` pairs are identical to each other, the bytes cannot tell which
one sits at a given address. These are recorded as `discovered` rather than
`documented`, with the alternative name written into the note.

`isinf` and `isnan` are absent from Ghidra's function map entirely — so this
method does not only supply names, it **also discovers missed functions.**

### Object-level placement was attempted — it does not work

Once a function's location is known, its object's base could be computed, and
from there every function in the object could fall out at once. This was
attempted; it did not work.

`make libc-align FUNC=remap_handle ADDR=0x0807180C` demonstrates it:

```
findslot                     0x080717EC     30  26/26 EXACT
remap_handle                 0x0807180C     76  60/60 EXACT
initialise_monitor_handles   0x08071858    112  86/92
get_errno                    0x080718C8     18  1/18
wrap                         0x080718F0     24  0/24
...
```

The first two functions match exactly, the third partially, and everything after
that drifts completely. The reason: **the ROM's newlib was built from the same
source but with a different configuration.** Configuration-independent helpers
such as `findslot`/`remap_handle` come out identical, while the system call stubs
(`_read`, `_write`, `_open`, `_sbrk`) were written specifically for the game
because the GBA has no operating system, and their sizes differ. Once one
function's size changes, everything after it shifts.

So **function-level masked search is the ceiling**; object-level placement is not
possible for this ROM. `make libc-align` remains as a diagnostic tool: it shows
which part of an object is shared and which part is game-specific.

The code tail (after `0x08070000`) is 69 functions / 6870 bytes, 2.4% of the
total code body. The `0x0806F000-0x08071E00` band holds 74 functions / 10478
bytes.

## Developer identifier table

Between `0x03CEF38` and `0x03E30A4` there are **5032 identifiers** — the
developers' own names, not game text:

```
brief_multi3_5c    g_manana4_port1    start_asuka7
e_wantedlevel_last3    l_vehicletest3_rn1    grptrafficpolicelevel2
```

Prefix distribution: `e_` 1119, `l_` 661, `brief_` 321, `grp` 12, `loc` 6, other
2913. In the `brief_` region the records are 16 bytes apart — a fixed-size array.

This table helps name the mission/entity lookup system and recognize the code
tables that reference it.

## The Crawfish lineage — the retail ROM is an inherited codebase

The retail game was made by **Digital Eclipse**, but the ROM carries two separate
**Crawfish Interactive** traces:

1. **The `CRAWSAVE` signature.** `InitSaveSystem` (`0x0800082C`) compares the
   first 8 bytes of the EEPROM metadata character by character against
   `'C' 'R' 'A' 'W' 'S' 'A' 'V' 'E'`. The string is *not* stored contiguously in
   the ROM — it exists as constants embedded in the comparison instructions.
   (That is why a plain text search does not find it.)
2. **The vehicle physics debug menu.** The "Press A and B to toggle car physics
   test" feature that TCRF documented for the Crawfish prototype is still present
   in the retail ROM (see the table below).

According to TCRF the project was inherited from Crawfish, and Crawfish closed in
November 2002. These two traces indicate that Digital Eclipse **took over
Crawfish's codebase** rather than starting from scratch. So the prototype dated
16 April 2002, though "a different game," may come from the same engine lineage.

## Debug traces

`0x03E3554`: `!!! ASSERT cam(0x%x 0x%x 0x%x) %s : %d` — camera debugging.

**TCRF's addresses are for the US version; they are shifted in the European
ROM.** The European equivalents:

| Content | TCRF (US) | Europe (ours) |
|---|---|---|
| Debug menu start | `0x3E31A8` | `0x3E40BC` |
| `CAR PHYSICS TEST` | — | `0x3E4184` |
| `PLACEHOLDER` | `0x7C9624` | `0x7C99D4` |
| `FLAKEY CHECK ON` | `0x7C76DC` | `0x7C7A8C` |
| `SAY HELLO TO MR PAGER` | `0x3BC6B5` | `0x7C909C` |
| `KILL FRENZY` | `0x7C7CA4` | `0x7C8054` |
| `MULTI PLAYER` | `0x7C93AC` | `0x7C975C` |

### Vehicle physics field names — ready-made names for the struct

Starting at `0x03E4198` there is a **fixed array with an 8-byte stride**. These
are the developers' own field names for the vehicle structure; when the vehicle
subsystem is reached, real names can be used instead of `unk_14`:

| Address | Label | Address | Label |
|---|---|---|---|
| `0x03E4198` | `SPEED` | `0x03E41D8` | `ROLL` |
| `0x03E41A0` | `ACCEL` | `0x03E41E0` | `TILT` |
| `0x03E41A8` | `FACE` | `0x03E41E8` | `VEL` |
| `0x03E41B0` | `DIR` | `0x03E41F0` | `POS` |
| `0x03E41B8` | `ROTSP` | `0x03E41F8` | `HBRAKE` |
| `0x03E41C0` | `RADIUS` | `0x03E4200` | *(empty)* |
| `0x03E41C8` | `FCOLL` | `0x03E4208` | `HORN` |
| `0x03E41D0` | `ACOLL` | | |

At `0x03E40BC` there are also `CUSTOM 0` – `CUSTOM 10`, with a 15-byte stride.

There are also debug features reachable in-game (TCRF):

- **Cheat Mode:** during play, A + B + Start → "CHEAT MODE ON" and the
  character's coordinates on screen. The coordinate display is useful for dynamic
  verification.
- **Level Select:** at the main menu, Left, Right, Up, Down, L, R, then hold
  Start and press A. An arrow appears under the third menu option.

## Source file names: none

The `FILE=[%s]` format shows that `__FILE__` was passed, but the ROM contains no
source file path or extension. The asserts were most likely disabled in the
release build and the string arguments eliminated. Translation unit names cannot
be recovered this way.

## Other data addresses (TCRF)

| Address | Content |
|---|---|
| `0x349104` | Unused mission text |
| `0x3BC6B5` | "SAY HELLO TO MR PAGER" |
| `0x3CDB9C` | Cut multiplayer mode text |
| `0x7C7490` | Vehicle names (including cut ones) |
| `0x7C7D48` | Cut mission text |
| `0x7C88E4` | Multiplayer mission names |
| `0x7C949C` | Emergency vehicle names |

## The prototype — limited value

The prototype documented by TCRF is **Crawfish Interactive's technology demo
dated 16 April 2002**: a single small area, one taxi, a tuning screen, and the
vehicle physics test. It comes from the initial phase planned as a GTA III port;
the retail game was made by Digital Eclipse and its game content is entirely
different.

Even so, because of the lineage above, the engine code may be shared. Whether the
prototype carries a symbol table is unknown — it is dumped on Hidden Palace. If
it is examined, the priority order is: does it have a symbol/debug section, are
the `CRAWSAVE` signature and vehicle physics menu the same, and are there shared
function bodies.

Note: the prototype ROM does not enter this repository, and legal acquisition is
the user's responsibility.
