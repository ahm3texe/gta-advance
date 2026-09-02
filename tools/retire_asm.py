#!/usr/bin/env python3
"""Bir bolgeyi C build'ine cevirir ve assembly kaynagini emekli eder.

Once bolgenin tamaminin C'den eslestigini dogrular; eslesmezse hicbir sey
silmez. Sonra Makefile kuralini degistirir ve .s/.ld dosyalarini siler.

Kullanim:  python3 tools/retire_asm.py <module/name> <start_hex> <end_hex>
Ornek:     python3 tools/retire_asm.py ui/menu_graphics 0x1E30 0x1F04
"""
import re
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from agbcc_build import compile_and_link, rom_bytes  # noqa: E402

ROOT = Path(__file__).resolve().parent.parent


def main() -> None:
    if len(sys.argv) != 4:
        sys.exit(__doc__)
    key, start, end = sys.argv[1], int(sys.argv[2], 16), int(sys.argv[3], 16)
    module, name = key.split("/")

    source = ROOT / f"src/{module}/{name}.c"
    blob, _, _ = compile_and_link(source)
    region = rom_bytes()[start:end]
    if blob != region:
        sys.exit(f"HATA: bolge eslesmiyor ({len(blob)} vs {len(region)} byte). "
                 f"Hicbir sey silinmedi.")

    makefile = ROOT / "Makefile"
    text = makefile.read_text()
    pattern = re.compile(
        rf"build/{module}/{name}\.o: src/{module}/{name}\.s\n.*?"
        rf"build/{module}/{name}\.bin: build/{module}/{name}\.elf\n[^\n]*\n",
        re.S,
    )
    replacement = (
        f"# C kaynagindan uretiliyor: assembly karsiligi emekli edildi.\n"
        f"build/{module}/{name}.bin: src/{module}/{name}.c "
        f"data/functions.csv data/ram_map.csv\n"
        f"\t@mkdir -p build/{module}\n"
        f"\t@python3 tools/build_c.py $< $@\n"
    )
    if not pattern.search(text):
        sys.exit(f"HATA: Makefile'da build/{module}/{name} kurali bulunamadi.")
    makefile.write_text(pattern.sub(replacement, text, count=1))

    for path in (ROOT / f"src/{module}/{name}.s", ROOT / f"config/{name}.ld"):
        if path.exists():
            path.unlink()
    print(f"{key}: {len(region)} byte C'den eslesiyor, assembly emekli edildi.")


if __name__ == "__main__":
    main()
