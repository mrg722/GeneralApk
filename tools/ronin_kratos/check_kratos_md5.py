#!/usr/bin/env python3
"""Verifica que RONIN-KRATOS no toco nada de Kratos ni del RONIN actual:
compara el md5 de data/kratos/kratos_*.txt, data/kratos/ronin.txt y de cada
assets/kratos/actor/*.png que NO sea de RONIN-KRATOS (ids < 9500) contra la
version de una referencia git (por defecto HEAD).  Uso: check_kratos_md5.py [ref]"""
import hashlib, subprocess, sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
ref = sys.argv[1] if len(sys.argv) > 1 else "HEAD"
files = [f"data/kratos/kratos_{w}.txt" for w in ("espadas", "cesto", "rayo", "garras")] + ["data/kratos/ronin.txt"]
tree = subprocess.run(["git", "-C", str(ROOT), "ls-tree", "--name-only", ref, "assets/kratos/actor/"],
                      capture_output=True, text=True, check=True).stdout.split()
files += [f for f in tree if not (Path(f).stem.isdigit() and int(Path(f).stem) >= 9500)]
bad = 0
for f in files:
    a = hashlib.md5((ROOT / f).read_bytes()).hexdigest()
    b = hashlib.md5(subprocess.run(["git", "-C", str(ROOT), "show", f"{ref}:{f}"], capture_output=True, check=True).stdout).hexdigest()
    if a != b:
        bad += 1
        print("DISTINTO", f)
print(f"md5: {len(files)} archivos de Kratos/RONIN comparados contra {ref}, distintos: {bad}")
sys.exit(1 if bad else 0)
