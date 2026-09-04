#!/usr/bin/env python3
"""Yeni bir C kaynagini dogrulanmis ROM bolgesi olarak kaydeder.

Once bolgenin tamaminin C'den eslestigini dogrular; eslesmezse hicbir sey
yazmaz. Sonra data/matching_regions.csv'ye giris ve Makefile'a build kurali
ekler, boylece `make matching` bundan sonra bolgeyi denetler.

retire_asm.py mevcut bir assembly bolgesini C'ye cevirir; bu arac ise ONCEDEN
HIC DOGRULANMAMIS bir ROM alanini kapsama katar -- yani gercek buyume budur.

Kullanim:  python3 tools/add_c_region.py <module/name> <start_hex> <end_hex> "<not>"
"""
import csv
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from agbcc_build import compile_and_link, rom_bytes  # noqa: E402

ROOT = Path(__file__).resolve().parent.parent
REGIONS = ROOT / "data/matching_regions.csv"
MAKEFILE = ROOT / "Makefile"


def main() -> None:
    if len(sys.argv) != 5:
        sys.exit(__doc__)
    key, start, end, note = sys.argv[1], int(sys.argv[2], 16), int(sys.argv[3], 16), sys.argv[4]
    module, name = key.split("/")

    source = ROOT / f"src/{module}/{name}.c"
    blob, _, _ = compile_and_link(source)
    region = rom_bytes()[start:end]
    if blob != region:
        sys.exit(f"HATA: bolge eslesmiyor ({len(blob)} vs {len(region)} byte). "
                 f"Hicbir sey yazilmadi.")

    binary = f"build/{module}/{name}.bin"
    rows = list(csv.DictReader(REGIONS.open(newline="", encoding="utf-8")))
    if any(r["binary"] == binary for r in rows):
        sys.exit(f"HATA: {binary} zaten matching_regions.csv icinde.")
    rows.append({
        "start": f"0x{0x08000000 + start:08X}",
        "end": f"0x{0x08000000 + end:08X}",
        "binary": binary,
        "module": module,
        "notes": note,
    })
    fields = ["start", "end", "binary", "module", "notes"]
    with REGIONS.open("w", newline="", encoding="utf-8") as handle:
        writer = csv.DictWriter(handle, fieldnames=fields, lineterminator="\n")
        writer.writeheader()
        for row in sorted(rows, key=lambda r: int(r["start"], 16)):
            writer.writerow({k: row.get(k, "") for k in fields})

    text = MAKEFILE.read_text()
    target = f"{module}-{name}-match".replace("_", "-")
    rule = (
        f"\nbuild/{module}/{name}.bin: src/{module}/{name}.c "
        f"data/functions.csv data/ram_map.csv\n"
        f"\t@mkdir -p build/{module}\n"
        f"\t@python3 tools/build_c.py $< $@\n\n"
        f"{target}: verify-rom build/{module}/{name}.bin\n"
        f"\t@python3 tools/compare_slice.py baserom.gba {start:#x} build/{module}/{name}.bin\n"
    )
    marker = "\nmatching: libc-verify"
    if marker not in text:
        sys.exit("HATA: Makefile'da 'matching:' hedefi bulunamadi.")
    text = text.replace(marker, rule + marker, 1)
    text = text.replace(marker, f"{marker[:-1]} {target}", 1) if False else text
    # matching hedefinin bagimlilik listesine ekle
    lines = text.splitlines(keepends=True)
    for i, line in enumerate(lines):
        if line.startswith("matching: libc-verify"):
            lines[i] = line.rstrip("\n") + f" {target}\n"
            break
    MAKEFILE.write_text("".join(lines))
    print(f"{key}: {len(region)} byte yeni dogrulanmis bolge olarak eklendi ({target}).")


if __name__ == "__main__":
    main()
