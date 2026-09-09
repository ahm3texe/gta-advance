#!/usr/bin/env python3
"""agbcc'nin register dagitim dokumunu okur ve pseudo -> DONANIM YAZMACI
haritasini basar.

NEDEN VAR
---------
Bu turun asil silahi kural 50 (YAZMAC ONCELIGI): ROM'un neden r4'u bir
degiskene verdigini, bizim derlememizin neden baskasina verdigini
bilmeden byte-matching'e ulasilamiyor.  Onceki surum yalnizca dokumun
BASINDAKI oncelik tablosunu okuyordu; "hangi C degiskenim r4'u kapti"
sorusunun tek satirlik cevabi ise dokumun ALTINDAKI `;; Register
dispositions` bolumunde yaziyor.  Uc fonksiyon o bolum basilmadigi icin
korlemesine kurcalandi.  Bu surum tum bolumleri okuyor.

HANGI BOLUMLERI OKUYOR (agbcc `-dg` -> `<girdi>.greg`)
-----------------------------------------------------
  "Registers to be allocated in sorted order:"
      her pseudo icin refs / live_length; ONCELIGE gore azalan sirali.
  ";; N regs to allocate: ..."
      GLOBAL dagiticinin gercekten isledigi allocno SIRASI.  Listede
      olmayan pseudo'lari yerel dagitici (local-alloc) blok icinde
      hallediyor; onlar global cakisma yarisina hic girmiyor.
  ";; N conflicts: ..."
      cakisma cizgesi; donanim yazmaclari da (0=r0 ... 13=sp) dahil.
  "Register N used X times across Y insns; ...; crosses K calls; pointer"
      kullanici degiskeni mi, kac cagriyi asiyor, isaretci mi, hangi
      bloga hapis.
  ";; Register dispositions:"   <<< KRITIK
      pseudo -> donanim yazmaci haritasi.  Burada olmayan pseudo yigina
      tasmis (spill) demektir.
  ";; Hard regs used:"
      fonksiyonun toplam yazmac ayak izi.

Ek olarak `-dr` dokumu (`<girdi>.rtl`, ilk RTL) okunuyor: her pseudo'nun
ILK TANIM yerini sadelestirip basiyoruz.  Bu, isimsiz pseudo 26'nin
aslinda `mem[p22+40]` oldugunu gosterir.

KURAL 50 ILE ILISKISI
---------------------
    oncelik = floor_log2(refs) * refs / omur      (esitlikte kucuk pseudo once)
Sira geldiginde find_reg, cakisma cizgesindeki EN KUCUK BOS yazmaci
verir; allocno bir cagriyi asiyorsa yalnizca callee-saved (r4+) adaylara
bakilir.  floor_log2 bir BASAMAK fonksiyonu oldugu icin bir degiskeni
ikiye bolmek onceligi ucte bire indirir ve sirayi degistirir.
KESIN SINIR: bolme yalnizca degerin IKI AYRI URETIM YERI varsa yeni
allocno uretir; kopya tabanli bolme (`lst = list;`) her zaman eleniyor.

C DEGISKEN ADLARI
-----------------
agbcc bu dokumlere degisken adi YAZMIYOR (`-g` destegi yok, stabs
uretmiyor).  Bu yuzden:
  - PARAMETRELER isimlendiriliyor: prologda `(set (reg/v N) (reg H rH))`
    kalibi arg sirasini kesin veriyor, ad da kaynaktaki imzadan okunuyor.
  - Yerel degiskenler icin ad "?" olarak birakiliyor ve yerine OLCULEN
    ilk tanim ifadesi basiliyor.  AD UYDURULMUYOR.

KULLANIM
--------
  python3 tools/dump_alloc.py <kaynak.c> [fonksiyon]      # eski kullanim
  python3 tools/dump_alloc.py <kaynak.c> [fonksiyon] --rom
  python3 tools/dump_alloc.py <kaynak.c> [fonksiyon] --conflicts

  --rom        ROM'daki ayni fonksiyonu cozup push listesini ve yazmac
               operand sayimlarini yaninda gosterir ("ROM ne istiyor").
  --conflicts  cakisma cizgesini de basar.
"""
import re
import subprocess
import sys
import tempfile
from math import floor, log2
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(Path(__file__).resolve().parent))

AGBCC_DIR = ROOT / "tools/agbcc/bin"
AGBCC = AGBCC_DIR / "old_agbcc"
ARM_AGBCC = AGBCC_DIR / "agbcc_arm"
ARM_MARKER = "MODE: ARM"
FLAGS = ["-mthumb-interwork", "-O2", "-fhex-asm"]
ARM_FLAGS = ["-mthumb-interwork", "-O2", "-fomit-frame-pointer",
             "-fno-schedule-insns", "-fno-schedule-insns2"]

# Thumb'da r0-r3 cagri-asindirilir (caller-saved), r4-r7 callee-saved.
# r8+ yuksek yazmac: agbcc bunlari da callee-saved sayip push/pop eder.
CALLEE_SAVED_FROM = 4


def hard_name(n: int) -> str:
    return {13: "sp", 14: "lr", 15: "pc"}.get(n, f"r{n}")


def priority(refs: int, length: int) -> float:
    """Kural 50: floor_log2(refs) * refs / omur."""
    if refs > 1 and length:
        return floor(log2(refs)) * refs / length
    return 0.0


# --------------------------------------------------------------------------
# RTL sadelestirici: (mem/s:SI (plus:SI (reg/v:SI 22) (const_int 40)) 2)
#                 -> mem[p22+40]
# --------------------------------------------------------------------------
def sexp_parse(text: str, i: int = 0):
    """Tek bir s-ifadesini ayristirir; (deger, sonraki_indeks) doner."""
    while i < len(text) and text[i].isspace():
        i += 1
    if i >= len(text):
        return None, i
    if text[i] == "(":
        items, i = [], i + 1
        while i < len(text):
            while i < len(text) and text[i].isspace():
                i += 1
            if i < len(text) and text[i] == ")":
                return items, i + 1
            item, i = sexp_parse(text, i)
            if item is None:
                break
            items.append(item)
        return items, i
    if text[i] == '"':
        j = text.index('"', i + 1) if '"' in text[i + 1:] else len(text) - 1
        return text[i:j + 1], j + 1
    j = i
    while j < len(text) and not text[j].isspace() and text[j] not in "()":
        j += 1
    return text[i:j], j


def sexp_show(node) -> str:
    """RTL dugumunu okunur tek satira cevirir. Bilinmeyen kodu oldugu gibi
    birakir -- kimlik uydurmuyoruz."""
    if isinstance(node, str):
        return node
    if not node:
        return "()"
    # RTL kodlari bayrak tasiyabiliyor: `mem/u`, `reg/v`, `symbol_ref/u`.
    # Bayraklari at, yoksa her bayrakli bicim ham fallback'e dusuyor.
    code = node[0].split(":")[0].split("/")[0] if isinstance(node[0], str) else "?"
    args = node[1:]
    s = sexp_show
    if code == "reg":
        # (reg:SI 22)  ya da  (reg:SI 0 r0) -> donanim yazmaci
        if len(args) >= 2 and isinstance(args[1], str) and not args[1].isdigit():
            return args[1]
        return f"p{args[0]}" if args else "reg"
    if code == "const_int":
        return args[0] if args else "?"
    if code == "plus":
        return "+".join(s(a) for a in args[:2])
    if code == "minus":
        return "-".join(s(a) for a in args[:2])
    if code == "mult":
        return "*".join(s(a) for a in args[:2])
    if code == "mem":
        return f"mem[{s(args[0])}]" if args else "mem[]"
    if code == "subreg":
        return s(args[0]) if args else "subreg"
    if code == "zero_extend":
        return f"zext({s(args[0])})"
    if code == "sign_extend":
        return f"sext({s(args[0])})"
    if code == "and":
        return " & ".join(s(a) for a in args[:2])
    if code == "ior":
        return " | ".join(s(a) for a in args[:2])
    if code == "xor":
        return " ^ ".join(s(a) for a in args[:2])
    if code in ("ashift", "ashiftrt", "lshiftrt"):
        op = {"ashift": "<<", "ashiftrt": ">>", "lshiftrt": ">>>"}[code]
        return op.join(s(a) for a in args[:2])
    if code == "symbol_ref":
        # (symbol_ref/u:SI ("*.LC1")) -> arg bir listeye sarili olabiliyor
        a = args[0] if args else "sym"
        while isinstance(a, list) and a:
            a = a[0]
        return str(a).strip('"').lstrip("*")
    if code == "call":
        return f"call {s(args[0])}" if args else "call"
    if code == "label_ref":
        return "label"
    return f"{code}({', '.join(s(a) for a in args[:3])})"


def rtl_origins(rtl_text: str, func: str):
    """Fonksiyonun ilk RTL dokumunden pseudo -> (arg_index, ilk_tanim) cikarir.

    arg_index: prologda `(set (reg/v N) (reg H rH))` ile dolduruluyorsa H,
    yoksa None.  Bu kalip parametre sirasini KESIN verir.
    """
    block = None
    for part in rtl_text.split(";; Function ")[1:]:
        if part.split("\n", 1)[0].strip() == func:
            block = part
            break
    if block is None:
        return {}, {}

    args, origin = {}, {}
    prologue = True
    # Parametre kopyalari NOTE_INSN_FUNCTION_BEG'den ONCE duruyor; o notu
    # gormek icin note'lar da taranmali, yoksa cagri donus degerleri de
    # "parametre" sanilir (olculdu: FUN_08052ddc'de pseudo 25 boyle
    # yanlislikla arg0 goruluyordu).
    for chunk in re.findall(
            r"^\((?:insn|jump_insn|call_insn|note)\b.*?(?=\n\n|\Z)",
            block, re.S | re.M):
        if chunk.startswith("(note"):
            if "NOTE_INSN_FUNCTION_BEG" in chunk:
                prologue = False
            continue
        node, _ = sexp_parse(chunk)
        if not isinstance(node, list) or len(node) < 5:
            continue
        body = node[4]
        if not (isinstance(body, list) and isinstance(body[0], str)
                and body[0].split(":")[0] == "set"):
            continue
        dest, src = body[1], body[2]
        if not (isinstance(dest, list) and isinstance(dest[0], str)
                and dest[0].split(":")[0].startswith("reg")):
            continue
        if len(dest) < 2 or not str(dest[1]).isdigit():
            continue
        pseudo = int(dest[1])
        if len(dest) >= 3 and isinstance(dest[2], str):
            continue                      # hedef donanim yazmaci, pseudo degil
        if pseudo not in origin:
            origin[pseudo] = sexp_show(src)
        # prologdaki donanim-yazmaci kopyasi = parametre
        if (prologue and pseudo not in args and isinstance(src, list)
                and isinstance(src[0], str)
                and src[0].split(":")[0].startswith("reg")
                and len(src) >= 3 and isinstance(src[2], str)):
            m = re.fullmatch(r"r(\d+)", src[2])
            if m and int(m.group(1)) <= 3:
                args[pseudo] = int(m.group(1))
    return args, origin


def param_names(source_text: str, func: str) -> list[str]:
    """Kaynaktaki TANIMDAN parametre adlarini okur. Cozemezse bos liste.

    Ad bir dosyada birden cok gecebiliyor (baslik notundaki eski imza,
    cagrilar, ileri bildirim).  Sadece kapanis parantezinden hemen sonra
    `{` gelen gecis TANIMDIR; olculdu: nodelist_a1.c'nin baslik notu eski
    ve YANLIS bir imza tasiyor, ilk gecise bakmak yanlis ad veriyordu.
    """
    inner = None
    for m in re.finditer(re.escape(func) + r"\s*\(", source_text):
        depth, close = 0, None
        for j in range(m.end() - 1, min(len(source_text), m.end() + 4000)):
            if source_text[j] == "(":
                depth += 1
            elif source_text[j] == ")":
                depth -= 1
                if depth == 0:
                    close = j
                    break
        if close is None:
            continue
        if re.match(r"\s*\{", source_text[close + 1:close + 40]):
            inner = source_text[m.end():close]
            break
    if inner is None:
        return []
    names, depth, cur = [], 0, ""
    for ch in inner:
        if ch in "([":
            depth += 1
        elif ch in ")]":
            depth -= 1
        if ch == "," and depth == 0:
            names.append(cur)
            cur = ""
        else:
            cur += ch
    names.append(cur)
    out = []
    for part in names:
        ids = re.findall(r"[A-Za-z_]\w*", part.replace("[", " ["))
        if not ids or part.strip() in ("void", ""):
            out.append("?")
        else:
            out.append(ids[-1])
    return out


# --------------------------------------------------------------------------
# .greg cozumleyicisi
# --------------------------------------------------------------------------
def parse_greg(block: str) -> dict:
    info = {}
    for reg, refs, length in re.findall(
            r"Register (\d+), refs = (\d+), live_length = (\d+)", block):
        info[int(reg)] = {
            "refs": int(refs), "life": int(length), "calls": 0,
            "user": False, "pointer": False, "block": None, "hw": None,
        }
    order = [int(x) for x in re.findall(r"\d+", m.group(1))] \
        if (m := re.search(r";; \d+ regs to allocate:([^\n]*)", block)) else []

    for line in re.findall(r"^Register (\d+) used .*$", block, re.M):
        pass
    for m in re.finditer(
            r"^Register (\d+) used (\d+) times across (\d+) insns([^\n]*)",
            block, re.M):
        reg, tail = int(m.group(1)), m.group(4)
        row = info.setdefault(reg, {"refs": int(m.group(2)),
                                    "life": int(m.group(3)), "calls": 0,
                                    "user": False, "pointer": False,
                                    "block": None, "hw": None})
        row["user"] = "user var" in tail
        row["pointer"] = "pointer" in tail
        c = re.search(r"crosses (\d+) call", tail)
        row["calls"] = int(c.group(1)) if c else 0
        b = re.search(r"in block (\d+)", tail)
        row["block"] = int(b.group(1)) if b else None

    disp = {}
    if (m := re.search(r";; Register dispositions:\n(.*?)\n\n", block, re.S)):
        for reg, hw in re.findall(r"(\d+) in (\d+)", m.group(1)):
            disp[int(reg)] = int(hw)
    for reg, hw in disp.items():
        info.setdefault(reg, {"refs": 0, "life": 0, "calls": 0, "user": False,
                              "pointer": False, "block": None, "hw": None})
        info[reg]["hw"] = hw

    conflicts = {}
    for m in re.finditer(r"^;; (\d+) conflicts:([^\n]*)", block, re.M):
        conflicts[int(m.group(1))] = [int(x) for x in re.findall(r"\d+", m.group(2))]

    hard = []
    if (m := re.search(r";; Hard regs used:([^\n]*)", block)):
        hard = [int(x) for x in re.findall(r"\d+", m.group(1))]

    return {
        "info": info,
        "order": order,
        "conflicts": conflicts,
        "hard": hard,
        "spills": len(re.findall(r"^Spilling for insn", block, re.M)),
        "total": int(m.group(1)) if (m := re.search(r"^(\d+) registers\.",
                                                    block, re.M)) else len(info),
    }


# --------------------------------------------------------------------------
# ROM tarafi (istege bagli)
# --------------------------------------------------------------------------
def rom_view(func: str):
    """ROM'daki fonksiyonu cozup (push_listesi, {yazmac: operand_sayisi})
    dondurur. Basarisiz olursa (None, hata_metni)."""
    try:
        from agbcc_build import ROM_BASE, function_rows, rom_bytes, run
    except Exception as exc:                                  # noqa: BLE001
        return None, f"agbcc_build alinamadi: {exc}"
    rows = function_rows()
    if func not in rows:
        return None, f"{func} data/functions.csv icinde yok"
    row = rows[func]
    address = int(row["address"], 16)
    size = int(row["size"] or 0)
    if not size:
        return None, f"{func} icin boyut yok"
    thumb = "ARM" not in (row.get("notes") or "").upper().split()
    blob = rom_bytes()[address - ROM_BASE:address - ROM_BASE + size]
    tmp = ROOT / "build"
    tmp.mkdir(parents=True, exist_ok=True)
    raw = tmp / "dump_alloc_rom.bin"
    raw.write_bytes(blob)
    out = run(["arm-none-eabi-objdump", "-b", "binary", "-m", "arm7tdmi",
               "-M", "force-thumb" if thumb else "no-force-thumb",
               "-D", f"--adjust-vma={address:#x}", str(raw)])
    return disasm_stats(out), None


def disasm_stats(text: str) -> dict:
    push, counts = None, {}
    for line in text.splitlines():
        m = re.match(r"\s*[0-9a-f]+:\s+[0-9a-f ]+\t(.*)", line)
        if not m:
            continue
        insn = re.sub(r"\s+", " ", m.group(1)).strip()
        if push is None and insn.startswith("push"):
            push = insn
        body = insn.split(";", 1)[0]
        for r in re.findall(r"\b(r\d+|sl|fp|ip|sp|lr|pc)\b", body):
            counts[r] = counts.get(r, 0) + 1
    return {"push": push, "counts": counts}


def mine_view(source: Path, func: str):
    """Kendi derlememizi ROM ile ayni bicimde cozer."""
    try:
        from agbcc_build import compile_and_link, run
    except Exception as exc:                                  # noqa: BLE001
        return None, f"agbcc_build alinamadi: {exc}"
    blob, layout, base = compile_and_link(source)
    if func not in layout:
        return None, f"{func} derlenmis ciktida yok"
    offset, size = layout[func]
    tmp = ROOT / "build"
    raw = tmp / "dump_alloc_mine.bin"
    raw.write_bytes(blob[offset:offset + size])
    out = run(["arm-none-eabi-objdump", "-b", "binary", "-m", "arm7tdmi",
               "-M", "force-thumb", "-D",
               f"--adjust-vma={base + offset:#x}", str(raw)])
    return disasm_stats(out), None


# --------------------------------------------------------------------------
def emit(name: str, data: dict, args: dict, origin: dict, pnames: list,
         want_conflicts: bool) -> None:
    info, order = data["info"], data["order"]

    def label(reg: int) -> str:
        if reg in args:
            idx = args[reg]
            nm = pnames[idx] if idx < len(pnames) else "?"
            return f"arg{idx} {nm}" if nm != "?" else f"arg{idx}"
        return "?"

    print(f"\n=== {name}: {len(info)} pseudo-register, {data['spills']} spill")
    hard = " ".join(hard_name(h) for h in data["hard"])
    saved = [h for h in data["hard"] if CALLEE_SAVED_FROM <= h <= 11]
    print(f"    donanim yazmaclari: {hard}"
          f"    (callee-saved: {len(saved)})")
    if not info:
        return

    ordered = sorted(info, key=lambda r: (-priority(info[r]["refs"],
                                                    info[r]["life"]), r))
    rank = {r: i + 1 for i, r in enumerate(order)}

    head = (f"  {'pseudo':>6} {'->HW':>5} {'refs':>5} {'omur':>5} {'oncelik':>8} "
            f"{'sira':>5} {'cagri':>5} {'ptr':>4} {'ad':<12} ilk tanim")
    print("\n  DAGITIM TABLOSU (oncelige gore azalan; 'sira' = global "
          "dagiticinin isleme sirasi)")
    print(head)
    print("  " + "-" * (len(head) - 2))
    for reg in ordered:
        row = info[reg]
        pri = priority(row["refs"], row["life"])
        hw = hard_name(row["hw"]) if row["hw"] is not None else "SPILL"
        star = "*" if row["hw"] is not None and row["hw"] >= CALLEE_SAVED_FROM \
            and row["hw"] <= 11 else " "
        sira = str(rank.get(reg, "")) if reg in rank else \
            (f"L{row['block']}" if row["block"] is not None else "-")
        print(f"  {reg:>6} {hw+star:>5} {row['refs']:>5} {row['life']:>5} "
              f"{pri:>8.3f} {sira:>5} {row['calls']:>5} "
              f"{('e' if row['pointer'] else '-'):>4} "
              f"{label(reg):<12} {origin.get(reg, '')[:46]}")
    print("  ('*' = callee-saved; 'sira' L<n> = yerel dagitici, blok n; "
          "'-' = global yarisa girmiyor)")

    print("\n  YAZMACA GORE (bir bakista)")
    by_hw: dict = {}
    for reg, row in info.items():
        by_hw.setdefault(row["hw"], []).append(reg)
    for hw in sorted(by_hw, key=lambda h: (h is None, h)):
        regs = sorted(by_hw[hw])
        tag = hard_name(hw) if hw is not None else "SPILL"
        detail = ", ".join(
            f"{r}({label(r) if label(r) != '?' else (origin.get(r) or '?')})"
            for r in regs)
        print(f"    {tag:>5}: {detail}")

    if want_conflicts and data["conflicts"]:
        print("\n  CAKISMA CIZGESI (donanim yazmaclari dahil)")
        for reg in sorted(data["conflicts"]):
            others = " ".join(str(x) for x in data["conflicts"][reg])
            print(f"    {reg:>4}: {others}")


def emit_rom(name: str, rom: dict, mine: dict) -> None:
    print(f"\n  ROM KARSILASTIRMASI ({name})")
    print(f"    ROM  push: {rom['push']}")
    print(f"    benim push: {mine['push'] if mine else '(derlenemedi)'}")
    keys = sorted(set(rom["counts"]) | set((mine or {}).get("counts", {})),
                  key=lambda r: (len(r), r))
    print(f"    {'yazmac':>7} {'ROM':>5} {'benim':>6}  operand sayisi")
    for r in keys:
        a = rom["counts"].get(r, 0)
        b = (mine or {}).get("counts", {}).get(r, 0)
        mark = "" if a == b else "   <-- fark"
        print(f"    {r:>7} {a:>5} {b:>6}{mark}")


def main() -> None:
    argv = [a for a in sys.argv[1:] if not a.startswith("--")]
    opts = {a for a in sys.argv[1:] if a.startswith("--")}
    if not argv:
        sys.exit(__doc__)
    source = Path(argv[0])
    want = argv[1] if len(argv) > 1 else None
    want_rom = "--rom" in opts
    want_conflicts = "--conflicts" in opts

    source_text = source.read_text(encoding="utf-8")
    is_arm = ARM_MARKER in source_text
    cc = ARM_AGBCC if is_arm else AGBCC
    flags = ARM_FLAGS if is_arm else FLAGS
    if not cc.exists():
        sys.exit(f"{cc.name} kurulu degil. Once: make agbcc")

    with tempfile.TemporaryDirectory() as d:
        work = Path(d)
        pre = work / "in.i"
        result = subprocess.run(
            ["cpp", "-nostdinc", "-undef", f"-I{ROOT / 'include'}", str(source)],
            capture_output=True, text=True)
        if result.returncode:
            sys.exit(f"cpp basarisiz:\n{result.stderr[:400]}")
        pre.write_text(result.stdout)
        result = subprocess.run(
            [str(cc), *flags, "-dg", "-dr", "-o", str(work / "out.s"), str(pre)],
            capture_output=True, text=True)
        if result.returncode:
            sys.exit(f"{cc.name} basarisiz:\n{result.stderr[:400]}")
        greg = work / "in.i.greg"
        if not greg.exists():
            sys.exit("dokum uretilmedi")
        text = greg.read_text()
        rtl = work / "in.i.rtl"
        rtl_text = rtl.read_text() if rtl.exists() else ""

    print(f"{source}  [{cc.name} {' '.join(flags)}]")
    seen = False
    for block in text.split(";; Function ")[1:]:
        name = block.split("\n", 1)[0].strip()
        if want and name != want:
            continue
        seen = True
        data = parse_greg(block)
        args, origin = rtl_origins(rtl_text, name)
        pnames = param_names(source_text, name)
        emit(name, data, args, origin, pnames, want_conflicts)
        if want_rom:
            rom, err = rom_view(name)
            if err:
                print(f"\n  ROM KARSILASTIRMASI atlandi: {err}")
            else:
                mine, merr = mine_view(source, name)
                if merr:
                    print(f"  (kendi ciktim cozulemedi: {merr})")
                    mine = None
                emit_rom(name, rom, mine)
    if not seen:
        sys.exit(f"{want} dokumde yok")


if __name__ == "__main__":
    main()
