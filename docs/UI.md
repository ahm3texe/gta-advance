# Initial user interface map

This note records the initial UI analysis. Matching states and provisional names
below describe that research stage; see [STATUS.md](STATUS.md) and the function
map for current results.

Immediately after the save/serialization layer, a function at `0x0800114C`
filters and positions menu items.

## `BuildActiveMenuItems` — `0x0800114C`

- Reads the item count at `+0x14` in the input structure.
- Iterates over 32-byte item records starting at `+0x18`.
- Filters active items using an optional visibility mask.
- Writes active item pointers to the array at `0x020011B0` and their count to `0x020011A0`.
- Computes menu width from at most eight items and writes the horizontal position to `0x02001414`.
- Delegates the final layout step to function `0x08001E1C`.

The name is provisional and behavior-based. The function and literal pool match
160/160 bytes across `0x0800114C–0x080011EB`.

## `DrawMenuItems` — `0x080011EC`

Draws the active item list with a different style for the selected item, splits
numeric values into decimal digits, and converts them to text prefixed by `$`.
The function and literal pools match 448/448 bytes across `0x080011EC–0x080013AB`.

## `InitMenuScreen` — `0x080013AC`

Configures blend registers, transfers palette data to palette RAM using DMA3,
clears VRAM, and initializes menu-related subsystems. The function and literal
pool match 172/172 bytes across `0x080013AC–0x08001457`.

## `RunMenuScreen` — `0x08001458`

Initial Ghidra analysis indicates that this large function builds the menu tree
according to the menu ID, processes input bits, enters and leaves submenus, and
calls `DrawMenuItems` when the selection changes. At that stage, the name and
precise boundaries were provisional and the function was not yet matching.

The `ResetMenuState`, `IsMenuFlagSet`, and `FinalizeMenuLayout` helpers match
112/112 bytes across `0x08001DC0–0x08001E2F`.

`LoadMenuGraphics`, `ClearMenuVram`, and two small display-control wrappers match
212/212 bytes across `0x08001E30–0x08001F03`. This block decompresses the menu
graphics resource, loads two palette ranges with DMA3, fills VRAM, and applies
`DISPCNT=0x0101`.
