#!/usr/bin/env python3
"""
Glane — Copyright 2026 Baptiste Faisy. Licence Apache-2.0.

glane_trace.py — matrice de traçabilité des exigences (idée C8).

Relie chaque exigence GLA-01..24 du cahier des charges aux fichiers de code et
de test qui la citent, et génère une matrice Markdown/HTML de preuves. C'est
l'ossature du dossier de soumission : chaque exigence satisfaite pointe vers
son test. 100 % stdlib.
"""
import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
CAHIER = os.path.join(ROOT, "docs", "GLANE-cahier-des-charges.md")
SCAN_DIRS = ["core/src", "core/tests", "host", "host/tests", "scripts", "tools"]
SCAN_EXT = {".c", ".h", ".py"}
GLA_RE = re.compile(r"GLA-\d{2}")


def load_requirements():
    """Extrait la liste des exigences (id, niveau, texte) du cahier."""
    reqs = []
    if not os.path.exists(CAHIER):
        return reqs
    with open(CAHIER, "r", encoding="utf-8") as f:
        for line in f:
            m = re.match(r"\|\s*(GLA-\d{2})\s*\|\s*([SCX])\s*\|\s*([^|]+)\|", line)
            if m:
                reqs.append((m.group(1), m.group(2), m.group(3).strip()))
    return reqs


def scan_references():
    """Retourne {GLA-NN: [fichiers relatifs qui le citent]}."""
    refs = {}
    for d in SCAN_DIRS:
        absd = os.path.join(ROOT, d)
        if not os.path.isdir(absd):
            continue
        for fn in sorted(os.listdir(absd)):
            ext = os.path.splitext(fn)[1]
            if ext not in SCAN_EXT:
                continue
            path = os.path.join(absd, fn)
            rel = os.path.relpath(path, ROOT)
            with open(path, "r", encoding="utf-8", errors="replace") as f:
                text = f.read()
            for gla in set(GLA_RE.findall(text)):
                refs.setdefault(gla, []).append(rel)
    return refs


def build_matrix():
    reqs = load_requirements()
    refs = scan_references()
    rows = []
    for gla, level, text in reqs:
        files = sorted(refs.get(gla, []))
        tests = [f for f in files if "test" in os.path.basename(f).lower()]
        rows.append({
            "id": gla, "level": level, "text": text,
            "files": files, "tests": tests,
            "covered": bool(tests),
        })
    return rows


def to_markdown(rows):
    out = ["# Glane — matrice de traçabilité GLA", "",
           "| Exigence | Niv. | Couverte | Tests / fichiers |",
           "|---|---|---|---|"]
    for r in rows:
        mark = "✅" if r["covered"] else ("—" if r["files"] else "❌")
        files = ", ".join(f"`{f}`" for f in r["files"]) or "_(aucun)_"
        out.append(f"| {r['id']} | {r['level']} | {mark} | {files} |")
    covered = sum(1 for r in rows if r["covered"])
    out += ["", f"**{covered}/{len(rows)} exigences citées dans un test.**"]
    return "\n".join(out)


def main(argv):
    fmt = argv[1] if len(argv) > 1 else "--md"
    rows = build_matrix()
    if fmt == "--json":
        import json
        print(json.dumps(rows, indent=2, ensure_ascii=False))
    else:
        print(to_markdown(rows))
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))