# Text and language data map

Structural findings. The text **itself** does not enter this repository (see
docs/ROADMAP.md: the ROM and assets extracted from it are not shared). Recorded
here is only where it lives, how it is organized, and how to read it.

## Summary

| What | Where |
|----|--------|
| Text body | `0x07B0FE0 - 0x07D7FC8` (~62 KB printable) |
| Pointer table | `0x0EC46D4 - 0x0EC771C` (**3090 entries**, all valid) |
| Level/entity name table | `0x03D0000 - 0x03DFFEC` (~4000 names) |

## Pointer table

3090 entries, all pointing into the text body, and **exactly divisible by five:
618 x 5**. The game's startup language screen offers FIVE options (trace log:
`gActiveMenuItemCount` 0->5, see docs/GAME_FLOW.md), so 618 text strings x 5
languages.

The slice -> language mapping is CONFIRMED (session 7): **slice k = language k**,
directly. It was tested by reading the value at the same index across all five
slices:

| Slice | Index 0 | Language |
|-------|----------|-----|
| 0 | ENGLISH  | English |
| 1 | *(SAME address as slice 0)* | Spanish |
| 2 | ANGLAIS  | French |
| 3 | INGLESE  | Italian |
| 4 | ENGLISCH | German |

The order matches `gLanguage` in the trace log exactly: as the player moved the
cursor down, the value advanced 0->1->2->3->4, and the menu order was
English/Spanish/French/Italian/German.

RETRACTED INFERENCE: it was previously claimed that "each slice's lowest pointer
falls just after that language's own name," leading to the belief that slice 0
was Italian, slice 1 French, and slice 3 German. That adjacency was
COINCIDENTAL; the real mapping is the direct one above.

The Spanish language names are not at index 0 but at indices 1, 2, 3, and 5;
slice 1's index 0 points at the same address as slice 0's (an unused slot).

## Encoding

The text is **LATIN-1 (ISO-8859-1)**, zero-terminated, uncompressed.
CORRECTION: it was first described as "plain ASCII," which was WRONG. Accented
characters occupy the 0x80-0xFF range: 0xC9 = E-acute, 0xD1 = N-tilde,
0xC1 = A-acute. That is why `ESPANOL` could not be found; in the ROM it is stored
as ESPAN(0xD1)OL. Menu labels are directly readable (`PRESS START` @ `0x07C6E70`,
`NEW GAME` @ `0x07C9748`, `LOAD GAME`, `ERASE`).

The ROM contains 5046 candidate LZ77 blocks, but the text region is not among
them; compression appears to be used for graphics and map data.

## OPEN QUESTIONS

- Some strings are SHARED across languages: slice 1's index 0 points at the same
  address as slice 0's. The number of shared strings has not been counted.
- Which of the 618 strings belongs to which screen is unknown. A trace script
  (tools/trace.lua) could capture which index is read as a dialogue opens.
- The suffixes in the level name table (`0x03D0000`) look like region codes
  (`a1`-`a7`, `c1`-`c3`, `k4`, `v1`), and the prefixes look like object types
  (`brief`, `briefing`, `pager`, `mafia`, `guard`, `ambush`, `playerstart`).
  This suggests the story is organized as briefing and pager messages.

## Reading

`tools/dump_text.py` runs locally, writes its output under `build/`, and does
NOT enter the repository.
