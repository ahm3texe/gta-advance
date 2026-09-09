#!/usr/bin/env python3
"""Set up a decomp-permuter working directory for a function.

Usage:  python3 tools/make_permuter_dir.py <source.c> <FunctionName> <address> <size>
Output: build/permuter/<FunctionName>/{base.c,target.o,settings.toml,compile.sh}

MAPPING SYMBOLS: the code body is `$t`, the literal pool `$d`. If the pool stays
inside `$t`, objdump decodes it as instructions and the permuter chases the wrong
target (measured on ClearTextArea: 66 apparent instructions, 61 real). The end
of the code is taken from tools/dump_cfg.py's block ranges.
"""
import re, subprocess, sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
ROM = ROOT / "baserom.gba"
ROM_BASE = 0x08000000

sys.path.insert(0, str(ROOT / "tools"))
from audit_boundaries import Walker  # noqa: E402

def regions(rom: bytes, addr: int, size: int):
    """A list of (kind, bytes): kind is 't' (code) or 'd' (pool). Resolved by
    recursive descent; the pool is separated correctly even MID-function."""
    w = Walker(rom, addr); w.run()
    out = []
    for a in range(addr, addr + size, 2):
        kind = 't' if a in w.code else 'd'
        if out and out[-1][0] == kind:
            out[-1][1] += rom[a - ROM_BASE:a - ROM_BASE + 2]
        else:
            out.append([kind, bytes(rom[a - ROM_BASE:a - ROM_BASE + 2])])
    return out

def main():
    src, name, addr, size = Path(sys.argv[1]), sys.argv[2], int(sys.argv[3], 16), int(sys.argv[4])
    work = ROOT / "build" / "permuter" / name
    work.mkdir(parents=True, exist_ok=True)
    rom = ROM.read_bytes()
    body = rom[addr - ROM_BASE: addr - ROM_BASE + size]
    lines = [".section .text", ".thumb", f".global {name}", ".align 2", "$t:", f"{name}:"]
    ncode = npool = 0
    for region_index, (kind, chunk) in enumerate(regions(rom, addr, size)):
        if kind == 't':
            lines.append(f"$t.region{region_index}:")
            for i in range(0, len(chunk), 2):
                lines.append(f"    .short {int.from_bytes(chunk[i:i+2], 'little'):#06x}")
            ncode += len(chunk)
        else:
            lines.append(f"$d.region{region_index}:")
            for i in range(0, len(chunk), 2):
                lines.append(f"    .short {int.from_bytes(chunk[i:i+2], 'little'):#06x}")
            npool += len(chunk)
    code = b"\0" * ncode; pool = b"\0" * npool   # for the summary only
    (work / "target.s").write_text("\n".join(lines) + "\n")
    subprocess.run(["arm-none-eabi-as", "-mcpu=arm7tdmi", "-mthumb-interwork",
                    "-o", str(work / "target.o"), str(work / "target.s")], check=True)

    # base.c: preprocessed, self-contained, with the body inside PERM_RANDOMIZE
    pre = subprocess.run(["cpp", "-nostdinc", "-undef", "-P", f"-I{ROOT/'include'}", str(src)],
                         capture_output=True, text=True, check=True).stdout
    m = re.search(rf"\b{re.escape(name)}\s*\([^)]*\)\s*\{{", pre)
    if not m:
        sys.exit(f"the body of {name} was not found")
    open_brace = m.end() - 1
    depth, i = 0, open_brace
    while True:
        if pre[i] == "{": depth += 1
        elif pre[i] == "}":
            depth -= 1
            if depth == 0: break
        i += 1
    base = pre[:open_brace+1] + "\nPERM_RANDOMIZE(\n" + pre[open_brace+1:i] + "\n)\n" + pre[i:]
    # pycparser does not know GCC's __inline__ keyword (recipe, step 3).
    base = base.replace("__inline__", "inline")
    (work / "base.c").write_text(base)

    (work / "settings.toml").write_text(f'compiler_type = "gcc"\nfunc_name = "{name}"\n')
    (work / "compile.sh").write_text(
        "#!/bin/sh\n" f'exec python3 "{ROOT}/tools/permuter_compile.py" "$@"\n')
    (work / "compile.sh").chmod(0o755)
    print(f"{name}: code {len(code)} B ({len(code)//2} insns) + pool {len(pool)} B -> {work.relative_to(ROOT)}")

if __name__ == "__main__":
    main()
