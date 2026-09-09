# Findings derived from external documents

## What this file is, and is not

Two development documents from 2003 were examined (a design/planning file and a
budget/milestone file). This file is **not the documents themselves**; it holds
the conclusions we drew from them and, wherever possible, **verified against the
ROM**.

The documents are producer-owned material that may not be redistributed. Under
the legal boundary in `docs/ROADMAP.md`, the documents, page images, and lengthy
quotations do **not** enter the repository. Every item here is an inference
written in our own words.

No banking details, signatures, or personal information from the documents has
been reproduced anywhere.

## VERIFIED against the ROM

**128 Mbit cartridge.** The document states this, and `baserom.gba` is exactly
16,777,216 bytes.

**Link-cable code exists in the ROM.** The document plans a 2-player link game.
The serial register constants were found in the literal pools: `SIOCNT`
(0x04000128) in six places, `SIODATA32` (0x04000120) and `RCNT` (0x04000134) in
one each. The functions carrying them: FUN_080657d8 (2374 B), SerialIrqHandler,
StepLinkFrame, InitLinkBlock, ServiceLinkFrame, ResetLinkHardware. About 3.4 KB
in total, and none of it has been written yet.

**Input polling lives in two functions.** `KEYINPUT` (0x04000130) appears seven
times in the literal pools; its owners are PollInput (228 B) and WaitForPartner
(320 B). Neither has been written yet.

## INFERENCE — the codebase comes from two teams

The invoices and milestone forms show that the project was started in 2002 by
**Crawfish Interactive** (the project name appears first as "Gang Wars", then as
"GTA Advanced") and handed over to **Digital Eclipse** in December 2002. The
second team's document is titled "planned improvements for the final product" and
treats the game as 50% complete at that date.

**What this means for us:** the ROM is not one team's consistent codebase. That
gives a plausible explanation for a puzzle we measured this session —
`FUN_080543D0` and `FindOrInitAreaNode` are near-twin functions, yet one compiled
as an entry-guarded `do/while` and the other as a rotated `for` (see the side
finding under COMPILER.md rule 49). It does not change the rule: **the loop form
must be read from the ROM for each function**, never copied from a sibling.

The documents also state that the second team contributed its own GBA library
components, that copyright in those components remained with them, and that they
would be removed from the delivered source. So part of the ROM may not be
game-specific but reusable library code — a third class resembling the `libc`
region. NOT YET VERIFIED.

## INFERENCE — the statistics counter cluster is explained

The document says roughly 35 statistics were planned for the pause-menu
statistics screen, tracking values such as distance travelled, hidden packages,
ammunition, and health.

The ROM already contains a cluster we matched between 0x080671B8 and 0x080673E0:
`BumpCount64`, `BumpCount68`, `BumpCount6A`, `BumpCount70`, `BumpCount7A`,
`BumpCount80`, `BumpCount84`, `BumpSaveCounter`, `AddDistance`,
`AccumulateDistance`, `ResetDistanceAccum`. We had named these "increments a
counter" — **without knowing which counter**.

The document identifies the cluster: they are the statistics-screen counters.
Which offset corresponds to which statistic is STILL UNKNOWN — that is resolved
by a game session (mGBA tracing), not by the document.

## INFERENCE — missions are compiled bytecode

The document says the second team reverse-engineered the mission scripts and
wrote a compiler for them. So the missions are **data** in the ROM, and there is
an **interpreter** that executes them.

That is a concrete target within the 183-function wall above 560 bytes: the
interpreter is most likely one of them. NOT YET SEARCHED FOR.

## INFERENCE — the audio driver lives in fast RAM

The document says "audio memory is normally moved to fast RAM" and worries that
3D processing may be consuming all of it. On the GBA, fast RAM is IWRAM
(0x03000000). That is where to look when searching for the audio driver.

## INFERENCE — 3D engine and fixed point

The document repeatedly mentions a "3d engine", "3d coordinates", and rounding
errors in fixed-point arithmetic. This supports the identity we assumed for
`BuildVolumePlanes` (0x0800AB88): a function that builds six face planes from an
eight-corner volume is 3D collision detection. It is consistent with the 20.12
and 16.16 fixed-point formats measured in this project.

There is also a hint for vehicle physics: at that time the cars' turning axis was
at the vehicle's center, and moving it between the front wheels was planned. That
is the marker to look for when searching for vehicle steering code. Which of the
two shipped is UNKNOWN.

## Control scheme — USE WITH CARE

The document gives a button mapping for on-foot and in-vehicle play: on foot, L
enters a vehicle, R jumps, A punches/fires, B runs; in a vehicle, L exits, R
fires, A accelerates, B brakes/reverses, A+B is the handbrake, and Select+Start
starts/cancels a vehicle mission.

**WARNING:** this is January 2003's "proposed revised scheme". The document
explicitly says that on-foot directional movement changed and that strafe mode
was removed, so this is NOT the scheme that was in the game at that moment. It
must not be assumed identical to the shipped game; when writing input functions,
read the masks from the ROM and treat this table only as a NAMING hint.

## NOT IN THE DOCUMENTS — keep expectations here

Hoped for but not found:
- struct layouts, field meanings, symbol names
- state-code or numeric-constant tables
- **the cartridge memory map** — it appears as a Milestone 3 deliverable in the
  document's own milestone list, but it is NOT among these documents

So these documents do not contribute directly to the matching percentage. Their
contribution is on the naming and subsystem-identification side.
