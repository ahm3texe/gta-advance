#!/usr/bin/env python3
"""Kurulu agbcc arac zincirini kilitli kimlik veya C-corpus ile dogrular.

Varsayilan hizli kip referans artifact SHA-256 degerlerini denetler.
`--corpus`, ikili hash'i host/build yoluna gore degisse bile derleyicinin sabit
temsil C corpus'u icin ayni baytlari urettigini kanitlar.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
LOCK = ROOT / "config/toolchain.lock.json"


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for block in iter(lambda: handle.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def artifact_check(lock: dict) -> bool:
    failures = []
    for relative, expected in lock["referenceArtifacts"].items():
        path = ROOT / relative
        actual = sha256(path) if path.exists() else "YOK"
        if actual != expected:
            failures.append((relative, expected, actual))

    if not failures:
        print(f"toolchain kimligi: TEMIZ ({len(lock['referenceArtifacts'])} artifact)")
        return True

    print("toolchain kimligi DEGISTI:", file=sys.stderr)
    for relative, expected, actual in failures:
        print(f"  {relative}\n    beklenen {expected}\n    bulunan  {actual}", file=sys.stderr)
    return False


def corpus_fingerprint(compiler: str) -> tuple[int, str]:
    sys.path.insert(0, str(ROOT / "tools"))
    from agbcc_build import compile_and_link  # pylint: disable=import-outside-toplevel

    digest = hashlib.sha256()
    count = 0
    lock = json.loads(LOCK.read_text(encoding="utf-8"))
    for relative in lock["compiler"]["corpusSources"]:
        source = ROOT / relative
        if not source.exists():
            sys.exit(f"toolchain corpus kaynagi yok: {relative}")
        blob, layout, _ = compile_and_link(source, compiler)
        for name, (offset, size) in sorted(layout.items()):
            digest.update(name.encode("utf-8"))
            digest.update(b"\0")
            digest.update(size.to_bytes(8, "little"))
            digest.update(blob[offset:offset + size])
            count += 1
    return count, digest.hexdigest()


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--corpus",
        action="store_true",
        help="artifact hash'i yerine tum C ciktisinin parmak izini dogrula",
    )
    args = parser.parse_args()
    lock = json.loads(LOCK.read_text(encoding="utf-8"))

    identity_ok = artifact_check(lock)
    if not args.corpus:
        return 0 if identity_ok else 1

    compiler = lock["compiler"]
    count, actual = corpus_fingerprint(compiler["name"])
    expected_count = compiler["corpusFunctionCount"]
    expected = compiler["corpusSha256"]
    if count != expected_count or actual != expected:
        print("toolchain C-corpus parmak izi TUTMUYOR:", file=sys.stderr)
        print(f"  fonksiyon: {count} (beklenen {expected_count})", file=sys.stderr)
        print(f"  parmak izi: {actual}\n  beklenen:   {expected}", file=sys.stderr)
        return 1
    print(f"toolchain C-corpus: TEMIZ ({count} fonksiyon, {actual[:12]}…)")
    if not identity_ok:
        print("Not: artifact hash'leri farkli, fakat uretilen C corpus birebir uyumlu.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
