# EEPROM save system

The ROM range `0x0800082C–0x08000C27` implements the game's high-level save layer.
Names are provisional symbols based on behavior, not original source symbols.

## Data layout

- Metadata RAM buffer: `0x02000ED0`, 32 bytes.
- First 8 bytes: ASCII signature `CRAWSAVE`.
- Byte `8`: configured slot count.
- Byte `16 + slot`: validity flag for the corresponding save slot.
- EEPROM metadata area: the first four 8-byte blocks (`0–3`).
- Slot data: starts at block `4 + slot * (payload_size / 8)`.

## Function map

| Address | Provisional name | Role |
|---|---|---|
| `0x0800082C` | `InitSaveSystem` | Initializes the EEPROM library, clamps the slot count to `1..16`, validates/creates the metadata signature, and aligns the save size to 8 bytes. |
| `0x0800091C` | `ReadEepromBytes` | Reads 8-byte blocks through the Nintendo EEPROM routine and transfers their byte sequence into the destination buffer. |
| `0x080009EC` | `WriteEepromBytes` | Writes and compares an 8-byte block, with up to 20 retries on failure. |
| `0x08000B00` | `WriteSaveSlot` | Checks slot/size bounds, writes the data, and sets the metadata validity flag. |
| `0x08000B78` | `ReadSaveSlot` | Checks the validity flag and reads slot data. |
| `0x08000BE4` | `IsSaveSlotValid` | Returns the slot-validity byte from metadata. |
| `0x08000C00` | `ReadSaveMetadata` | Reads the fixed 32-byte metadata area. |
| `0x08000C14` | `WriteSaveMetadata` | Writes the fixed 32-byte metadata area. |
| `0x08000C28` | `InitSaveManager` | Loads the three game-save headers and validates their marker/checksum pairs. |
| `0x08000D20` | `LoadSaveSlot` | Loads a 160-byte game save and checks its checksum. |
| `0x08000D80` | `WriteGameSaveSlot` | Builds a checksum using the marker and its complement, then writes a 160-byte slot. |
| `0x08000DDC` | `ReadEepromRange` | Reads an arbitrary, potentially unaligned EEPROM byte range. |
| `0x08000F1C` | `WriteEepromRange` | Writes an unaligned range with a bounded retry count per block. |
| `0x08001094` | `EraseSaveSlot` | Clears the slot header and its first EEPROM block. |
| `0x080010D4` | `GetSaveSlotHeader` | Returns a valid slot-header pointer or null. |

The save layer and its eight adjacent little-endian serialization helpers match
**2336/2336 bytes** continuously across `0x0800082C–0x0800114B`, including literal
pools and padding. Sources are under `src/save/`; `make matching` verifies all
registered pieces against the ROM.

The Nintendo EEPROM library appears later in the ROM with version tag
`EEPROM_V124`. High-level function names describe verified behavior; library
function boundaries and names are to be established separately.
