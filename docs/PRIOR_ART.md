# Prior research: existing work and reusable evidence

Last checked: 2026-09-02. This is a historical search report, not a new search.

## Findings

Searches of the public web, GitHub repositories, GitLab/web indexes, Data
Crystal/TCRF, and ROM-hacking resources found **no published matching
decompilation, disassembly repository, Ghidra project, or comprehensive ROM map
for Grand Theft Auto Advance**.

Most GTA Advance projects found were browser emulators/ROM bundles, remakes in
another GTA engine, or mods. They did not provide ARM source code or a symbol
map for this ROM.

This negative result does not prove that no work exists. Private Discord servers,
unindexed forum attachments, or personal archives may contain additional work.
New material should not be incorporated before its license and provenance have
been verified.

## Available public information

- A physical European cartridge record identifies `AGB-BGTP-EUR`, ROM part `MX23L12806-12C`, and an EEPROM component.
- A public cheat database lists RAM addresses for player health, armor, money, weapons, wanted level, and some mission counters. These are starting points for dynamic analysis.
- Community dialogue/one-line text dumps exist. They are not code maps, but can help validate text tables.
- A development interview states that Digital Eclipse rebuilt the game without code from the earlier canceled GBA projects. This is an interview claim and requires separate technical verification against the ROM.

## Evidence verified in the ROM

| ROM offset | GBA address | Finding | Meaning |
|---:|---:|---|---|
| `0x000000` | `0x08000000` | ARM branch | Target `0x080000C0` |
| `0x06BCF0` | `0x0806BCF0` | Thumb function entry | Part of the assertion/log call chain |
| `0x06BD18` | `0x0806BD18` | Literal reference to `0x08BD3470` | Code reference to the assertion format string |
| `0xBD3450` | `0x08BD3450` | `MultiSioSync020820` | Nintendo MultiSio library signature |
| `0xBD3470` | `0x08BD3470` | `ASSERTION FAILED ...` | SDK assertion format string |
| `0xBD34D4` | `0x08BD34D4` | `EEPROM_V124` | Nintendo EEPROM library version signature |

Code around `0x0806BCF0` consists of Thumb instructions. Treat its function
boundaries and names as provisional until disassembler analysis is complete.

## Provenance and licensing approach

Allegedly leaked private Rockstar/Digital Eclipse source code will not be used.
The project accepts analysis derived from the locally supplied ROM, original
reimplementations, explicitly licensed tools/code, and verifiable public
technical information.

## Sources

- Physical cartridge record: <https://gbhwdb.gekkio.fi/cartridges/AGB-BGTP-0/kurodo-1.html>
- Public libretro cheat data: <https://github.com/libretro/libretro-database/blob/master/cht/Nintendo%20-%20Game%20Boy%20Advance/Grand%20Theft%20Auto%20Advance%20%28USA%2C%20Europe%29%20%28Code%20Breaker%29.cht>
- Development interview: <https://www.timeextension.com/features/the-making-of-grand-theft-auto-advance-the-gta-iii-prequel-yourve-probably-never-heard-of>
- Dialogue data: <https://github.com/ThirteenAG/GTA-One-Liners>
