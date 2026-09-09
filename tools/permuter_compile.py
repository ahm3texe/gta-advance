#!/usr/bin/env python3
"""Compile a single C source to a .o for the permuter.

Why a separate tool: the "cc -c input.c -o output.o" form the permuter expects
does not represent our pipeline EXACTLY. Here external symbols are turned into
absolute addresses with `.equ` (the rationale is in agbcc_build.py: otherwise the
linker inserts an interworking veneer and breaks the `bl` target). Because the
permuter compiles mutated sources in temporary directories, this step must work
there too.

Usage: python3 tools/permuter_compile.py [-o <output.o>] <input.c>

The `-o` flag is REQUIRED but its POSITION is free. The permuter looks for `-o`
to recognize the build command as a COMPILER invocation (without it the candidate
is discarded); the compile.sh it generates calls the script in the order
`<input> -o <output>`. Both orders must be supported.
"""
import shutil
import subprocess
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from agbcc_build import (  # noqa: E402
    AGBCC_DIR, CC1FLAGS, DEFAULT_CC, ROOT, _undefined, function_rows, ram_rows,
)


def run(cmd, out=None):
    result = subprocess.run(cmd, capture_output=True, text=True)
    if result.returncode != 0:
        sys.exit(f"{cmd[0]} basarisiz:\n{result.stderr.strip()[:800]}")
    if out is not None:
        out.write_text(result.stdout, encoding="utf-8")


def main() -> None:
    args = sys.argv[1:]
    target_arg = None
    positional = []
    i = 0
    while i < len(args):
        if args[i] == "-o" and i + 1 < len(args):
            target_arg = args[i + 1]
            i += 2
            continue
        positional.append(args[i])
        i += 1
    if target_arg is None or len(positional) != 1:
        sys.exit(__doc__)
    target, source = Path(target_arg).resolve(), Path(positional[0]).resolve()
    target.parent.mkdir(parents=True, exist_ok=True)
    work = target.parent
    stem = work / source.stem
    agbcc = AGBCC_DIR / DEFAULT_CC

    run(["cpp", "-nostdinc", "-undef", f"-I{ROOT / 'include'}", str(source)],
        Path(f"{stem}.i"))
    run([str(agbcc), *CC1FLAGS, "-o", f"{stem}.s", f"{stem}.i"])
    run(["arm-none-eabi-as", "-mcpu=arm7tdmi", "-mthumb-interwork",
         "-o", f"{stem}.probe.o", f"{stem}.s"])

    rows, ram = function_rows(), ram_rows()
    externs = []
    for name in _undefined(Path(f"{stem}.probe.o")):
        # The `__thumb` suffix resolves to the symbol address | 1. Stored
        # function pointers must have the Thumb bit set; in a `bl` target
        # ise bit eklemek dal ofsetini bozar, o yuzden AYRI bir ad kullanilir.
        thumb = name.endswith("__thumb")
        key = name[: -len("__thumb")] if thumb else name
        row = rows.get(key) or ram.get(key)
        if row is None:
            sys.exit(f"'{name}' is in neither data/functions.csv nor data/ram_map.csv")
        value = int(row["address"], 16) | (1 if thumb else 0)
        externs.append(f"    .equ {name}, {value:#x}\n")
    text = Path(f"{stem}.s").read_text(encoding="utf-8")
    Path(f"{stem}.s").write_text(
        "".join(externs) + text + "\n    .align 2, 0\n", encoding="utf-8")

    run(["arm-none-eabi-as", "-mcpu=arm7tdmi", "-mthumb-interwork",
         "-o", f"{stem}.o", f"{stem}.s"])
    if Path(f"{stem}.o") != target:
        shutil.copy(Path(f"{stem}.o"), target)


if __name__ == "__main__":
    main()
