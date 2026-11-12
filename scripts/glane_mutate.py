#!/usr/bin/env python3
"""
Glane — Copyright 2026 Baptiste Faisy. Licence Apache-2.0.

glane_mutate.py — test de mutation du socle C (plan de vérification §7.5).

Injecte des mutations ciblées du code de décision (seuils, comparaisons,
arrondis, gestion de seq), reconstruit et rejoue les tests dans une copie
temporaire du dépôt. Un mutant est « tué » si au moins un test échoue. Toute
survie révèle un trou dans la suite de tests. Porte : tous les mutants doivent
être tués. 100 % stdlib.

Usage : python3 scripts/glane_mutate.py [--keep]
"""
import os
import shutil
import subprocess
import sys
import tempfile

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

# (fichier relatif, ancien, nouveau, description). Chaque `ancien` doit
# exister exactement dans le fichier (vérifié avant mutation).
MUTATIONS = [
    ("core/src/deadline.c",
     "if (age_ns <= d->stale_after_ns) {",
     "if (age_ns < d->stale_after_ns) {",
     "deadline: frontière STALE (< au lieu de <=)"),
    ("core/src/deadline.c",
     "if (d->missing >= d->fault_after) {",
     "if (d->missing > d->fault_after) {",
     "deadline: seuil FAULT décalé d'une période"),
    ("core/src/ring.c",
     "if (next != expected) {",
     "if (next == expected) {",
     "ring: détection de trou inversée"),
    ("core/src/sample.c",
     "if (ts_ns < s->ts_ns) {",
     "if (ts_ns <= s->ts_ns) {",
     "sample: rejoue les ts égaux (monotonie stricte)"),
    ("core/src/saturate.c",
     "if (v > (int64_t)INT32_MAX) {",
     "if (v >= (int64_t)INT32_MAX) {",
     "saturate: borne haute clampée trop tôt"),
    ("core/src/schema.c",
     "if (n >= GLANE_NAME_MAX_LOCAL) {",
     "if (n > GLANE_NAME_MAX_LOCAL) {",
     "schema: nom de 64 octets accepté à tort"),
    ("core/src/virtuel.c",
     "if (raw < v->cfg.sat_min_milli) {",
     "if (raw <= v->cfg.sat_min_milli) {",
     "virtuel: saturation basse déclenchée sur la borne"),
    ("core/src/desc_validate.c",
     "if (glane_unit_name(d->unit)[0] == '?')",
     "if (glane_unit_name(d->unit)[0] != '?')",
     "desc: validation d'unité inversée"),
    ("core/src/calib.c",
     "        return INT32_MAX;",
     "        return INT32_MAX - 1;",
     "calib: clamp haut renvoie INT32_MAX-1"),
    ("core/src/calib.c",
     "scaled = (sig + d / 2) / d;",
     "scaled = sig / d;",
     "calib: arrondi demi-à-l'infini supprimé (troncature)"),
]


def prepare_temp():
    """Copie le nécessaire pour builder dans un répertoire temporaire."""
    tmp = tempfile.mkdtemp(prefix="glane-mutate-")
    for d in ("core", "tools"):
        shutil.copytree(os.path.join(ROOT, d), os.path.join(tmp, d))
    shutil.copy(os.path.join(ROOT, "Makefile"), os.path.join(tmp, "Makefile"))
    return tmp


def run_tests(tmp):
    """Construit et joue les tests dans tmp. Retourne True si VERTS."""
    r = subprocess.run(["make", "test"], cwd=tmp,
                       stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL,
                       timeout=180)
    return r.returncode == 0


def main(argv):
    keep = "--keep" in argv
    # Vérifie que toutes les cibles existent avant de commencer.
    for rel, old, _new, _d in MUTATIONS:
        with open(os.path.join(ROOT, rel), "r", encoding="utf-8") as f:
            if old not in f.read():
                print(f"ERREUR: cible absente dans {rel}: {old!r}",
                      file=sys.stderr)
                return 2
    tmp = prepare_temp()
    killed, survived = 0, []
    try:
        for rel, old, new, desc in MUTATIONS:
            path = os.path.join(tmp, rel)
            with open(os.path.join(ROOT, rel), "r", encoding="utf-8") as f:
                original = f.read()
            mutated = original.replace(old, new, 1)
            with open(path, "w", encoding="utf-8") as f:
                f.write(mutated)
            green = run_tests(tmp)
            with open(path, "w", encoding="utf-8") as f:
                f.write(original)  # restaure avant le mutant suivant
            if green:
                survived.append(desc)
                print(f"SURVIE  : {desc}")
            else:
                killed += 1
                print(f"tué     : {desc}")
    finally:
        if keep:
            print(f"temp conservé: {tmp}")
        else:
            shutil.rmtree(tmp, ignore_errors=True)
    total = len(MUTATIONS)
    print(f"\n{killed}/{total} mutants tués")
    if survived:
        print("MUTATION FAIL — survivants :", file=sys.stderr)
        for s in survived:
            print("  " + s, file=sys.stderr)
        return 1
    print("MUTATION OK : tous les mutants tués")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))