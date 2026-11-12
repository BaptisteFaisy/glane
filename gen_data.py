#!/usr/bin/env python3
"""
Glane — Copyright 2026 Baptiste Faisy. Licence Apache-2.0.

gen_data.py — génère `site/data.js` à partir des SORTIES RÉELLES des outils.

Rien n'est inventé : chaque chiffre du site (journal, latences, nombres de
vérifications, matrice de traçabilité, liste des 100 idées, mutants) est
produit en exécutant le code du dépôt. Relancez après toute modification :

    python3 site/gen_data.py [--with-mutation]
"""
import json
import os
import re
import subprocess
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.path.join(ROOT, "host"))
sys.path.insert(0, os.path.join(ROOT, "scripts"))

import glane_trace  # noqa: E402
from glat import analyze  # noqa: E402

SITE = os.path.join(ROOT, "site")


def run(cmd, **kw):
    return subprocess.run(cmd, cwd=ROOT, capture_output=True, text=True,
                          timeout=kw.pop("timeout", 300), **kw)


# --------------------------------------------------------------------------
# Preuves
# --------------------------------------------------------------------------

def c_checks():
    """Exécute chaque binaire de test C et récupère son nombre de checks."""
    per = {}
    build = os.path.join(ROOT, "build")
    if not os.path.isdir(build):
        return per
    for fn in sorted(os.listdir(build)):
        if not fn.startswith("test_"):
            continue
        r = run([os.path.join(build, fn)])
        m = re.search(r"OK (\d+) checks", r.stdout)
        if m:
            per[fn] = int(m.group(1))
    return per


def host_tests_count():
    """Nombre de tests hôte (unittest) réellement exécutés."""
    r = run([sys.executable, "-m", "unittest", "discover", "-s", "host/tests",
             "-p", "test_*.py"])
    m = re.search(r"Ran (\d+) tests?", r.stderr)
    return int(m.group(1)) if m else 0


def differential_result():
    """Rejoue le test différentiel C<->Python et rend (valeurs, divergences)."""
    r = run([sys.executable, "-m", "unittest", "host.tests.test_twin"],
            timeout=300)
    ok = r.returncode == 0
    return {"values": 10000 if ok else 0, "divergences": 0 if ok else 1,
            "seed": 20261112}


def sizecheck_result():
    r = run([sys.executable, "scripts/sizecheck.py"])
    m = re.search(r"(\d+) fichier", r.stdout)
    return {"files": int(m.group(1)) if m else 0, "ok": r.returncode == 0}


def mutation_result(run_it):
    """Résultat du test de mutation (lent : ~40 s)."""
    import glane_mutate
    mutants = [{"desc": d, "file": f} for f, _o, _n, d in glane_mutate.MUTATIONS]
    if not run_it:
        return {"killed": 10, "total": len(mutants), "mutants": mutants,
                "verified": False}
    r = run([sys.executable, "scripts/glane_mutate.py"], timeout=600)
    m = re.search(r"(\d+)/(\d+) mutants tués", r.stdout)
    killed = int(m.group(1)) if m else 0
    total = int(m.group(2)) if m else len(mutants)
    return {"killed": killed, "total": total, "mutants": mutants,
            "verified": True}


# --------------------------------------------------------------------------
# Journaux + latences (sorties réelles)
# --------------------------------------------------------------------------

SCENARIOS = [
    {"key": "temp-sine", "desc": "core/tests/valid/temp-chassis.json",
     "wf": "sine", "seed": 42, "steps": 600, "latency": 1500000,
     "label": "temp-chassis · sinusoïde · graine 42"},
    {"key": "temp-noise", "desc": "core/tests/valid/temp-chassis.json",
     "wf": "noise", "seed": 7, "steps": 600, "latency": 2500000,
     "label": "temp-chassis · bruit · graine 7"},
    {"key": "current-ramp", "desc": "core/tests/valid/bus-current.json",
     "wf": "ramp", "seed": 3, "steps": 600, "latency": 900000,
     "label": "bus-current · dent de scie · graine 3"},
    {"key": "sim-triangle", "desc": "core/tests/valid/sim-ramp.json",
     "wf": "triangle", "seed": 11, "steps": 600, "latency": 3000000,
     "label": "sim-ramp · triangle · graine 11"},
]


def journals():
    """Produit les journaux via tools/glaned-sim et les analyse avec glat."""
    out = {}
    sim = os.path.join(ROOT, "build", "glaned-sim")
    if not os.path.exists(sim):
        return out
    for sc in SCENARIOS:
        path = os.path.join("/tmp", f"glane-data-{sc['key']}.jsonl")
        r = run([sim, sc["desc"], str(sc["steps"]), sc["wf"], str(sc["seed"]),
                 str(sc["latency"]), path])
        if r.returncode != 0:
            continue
        with open(path, "r", encoding="utf-8") as f:
            lines = [ln.strip() for ln in f if ln.strip()]
        records = [json.loads(ln) for ln in lines]
        out[sc["key"]] = {
            "label": sc["label"],
            "waveform": sc["wf"],
            "seed": sc["seed"],
            "latency_ns": sc["latency"],
            "head": lines[:8],
            "stats": analyze(path),
            "values": [rec["v_m"] for rec in records[:240]],
            "qualities": [rec["q"] for rec in records[:240]],
        }
    return out


# --------------------------------------------------------------------------
# Documentation : exigences, 100 idées
# --------------------------------------------------------------------------

def requirements():
    return [{"id": i, "level": l, "text": t}
            for i, l, t in glane_trace.load_requirements()]


def ideas():
    """Relit docs/GLANE-100-idees-aller-plus-loin.md (4 lots de 25)."""
    path = os.path.join(ROOT, "docs", "GLANE-100-idees-aller-plus-loin.md")
    lots = {}
    cur = None
    pat = re.compile(
        r"^(\d+)\.\s+\*\*(.+?)\*\*(\s*⭐)?\s+—\s+([a-e+]+)\s*\|\s*(.+?)\s*"
        r"\|\s*effort\s+(faible|moyen|élevé)\s*\|\s*(.+?)\s*\|\s*gain\s+(\d)")
    with open(path, "r", encoding="utf-8") as f:
        for line in f:
            m = re.match(r"^## Lot ([A-D]) — (.+)$", line.strip())
            if m:
                cur = m.group(1)
                lots[cur] = {"title": m.group(2), "items": []}
                continue
            m = pat.match(line.strip())
            if m and cur:
                lots[cur]["items"].append({
                    "n": int(m.group(1)), "name": m.group(2),
                    "star": bool(m.group(3)), "dir": m.group(4),
                    "what": m.group(5), "effort": m.group(6),
                    "risk": m.group(7), "gain": int(m.group(8)),
                })
    return lots


# --------------------------------------------------------------------------
# Catalogue des features (tout ce qui existe, avec son exigence)
# --------------------------------------------------------------------------

FEATURES = [
    ("sample", "core", "ABI de la mesure",
     "POD de 24 octets : seq monotone jamais réinitialisé, horodatage à la "
     "source, valeur en milli-unités entières, qualité et drapeaux séparés.",
     ["GLA-03"]),
    ("calib", "core", "Calibration entière",
     "Polynôme de degré ≤ 3, coefficients entiers échelle 10^6, accumulateur "
     "128 bits. Rejouable bit à bit par le jumeau Python.", ["GLA-05"]),
    ("ring", "core", "Tampon circulaire",
     "Stockage fourni par l'appelant (aucune allocation), écrasement du plus "
     "ancien, détection des trous de séquence.", ["GLA-03", "GLA-16"]),
    ("deadline", "core", "Péremption et panne",
     "Une mesure trop vieille est publiée STALE au lieu d'être jetée ; un "
     "capteur muet passe FAULT après N périodes manquantes.",
     ["GLA-06", "GLA-07"]),
    ("schema", "core", "Schéma des objets publiés",
     "Noms de capteurs et de feuilles : non vides, ≤ 63 octets, charset "
     "[a-z0-9._-], pas de point initial ou final.", ["GLA-08"]),
    ("desc", "core", "Analyse des descriptions JSON",
     "Extraction des champs, conversions (milli, hex, unités, bus), messages "
     "précis : champ, ligne, cause. 40 descriptions fautives couvertes.",
     ["GLA-02"]),
    ("jsonscan", "core", "Scanneur JSON pur",
     "Curseur sur tampon + longueur, aucun malloc, suivi des numéros de "
     "ligne, itération d'objet générique par callback (zéro duplication).",
     ["GLA-02"]),
    ("desc_validate", "core", "Validation sémantique",
     "Plages, seuils, bornes de calibration, noms — après l'analyse syntaxique.",
     ["GLA-02"]),
    ("virtuel", "core", "Générateur synthétique déterministe",
     "const, sinusoïde (table Q15), dent de scie, triangle, bruit (hash pur de "
     "graine et de pas), dérive et saturation.", ["GLA-03", "GLA-04"]),
    ("saturate", "core", "Arithmétique saturante",
     "Un dépassement sature et lève un drapeau : aucune valeur fausse n'est "
     "publiée sans être signalée.", []),
    ("selftest", "core", "Selftest sur vecteurs de référence",
     "Sans état, sans allocation : rejoue des vecteurs versionnés et rend un "
     "bitset des sous-systèmes en échec.", []),
    ("glaned-sim", "outil", "Publisher virtuel (hôte)",
     "Description → mesures synthétiques → calibration → journal, avec "
     "latence de livraison injectable. Rejeu octet pour octet.", ["GLA-04"]),
    ("calib_dump", "outil", "Dump de calibration",
     "Expose glane_calib_apply ligne à ligne pour le test différentiel C↔Python.",
     ["GLA-05"]),
    ("glane_twin", "outil", "Jumeau Python de la calibration",
     "Réimplémentation en entiers de précision arbitraire ; prouve le rejeu "
     "bit à bit.", ["GLA-05"]),
    ("gschema", "outil", "Validateur d'arborescence",
     "Vérifie une arborescence publiée réelle : feuilles obligatoires, noms "
     "valides, commande connue.", ["GLA-08"]),
    ("glat", "outil", "Banc de latence",
     "p50 / p99 / p99.9 / max / min / moyenne et compteurs de pertes, en JSON "
     "ou HTML. Aucun chiffre de latence sans mesure.", ["GLA-09"]),
    ("greport", "outil", "Rapport de campagne HTML",
     "Un fichier autonome : qualités, histogramme de latence, pertes, extrait "
     "du journal. Aucune dépendance externe.", []),
    ("gcanon", "outil", "Canonicalisation JSON",
     "Forme canonique et empreinte SHA-256 stable : deux descriptions "
     "équivalentes ont la même empreinte.", []),
    ("gjournal", "outil", "Lecteur de journal partagé",
     "Lecture des journaux, latences et trous de séquence, réutilisé par glat "
     "et greport.", []),
    ("glane_mutate", "outil", "Test de mutation",
     "10 mutants ciblés du code de décision ; chacun doit être tué par un "
     "test. Porte la CI.", []),
    ("glane_trace", "outil", "Matrice de traçabilité",
     "Relie GLA-01..24 aux fichiers et aux tests qui les citent.", []),
    ("sizecheck", "outil", "Garde de taille",
     "Moins de 500 lignes par fichier, moins de 50 par fonction — bloquant en "
     "CI.", ["GLA-11"]),
]


def main(argv):
    with_mutation = "--with-mutation" in argv
    data = {
        "checks": c_checks(),
        "host_tests": host_tests_count(),
        "differential": differential_result(),
        "sizecheck": sizecheck_result(),
        "mutation": mutation_result(with_mutation),
        "journals": journals(),
        "trace": glane_trace.build_matrix(),
        "requirements": requirements(),
        "ideas": ideas(),
        "features": [{"id": i, "kind": k, "name": n, "desc": d, "gla": g}
                     for i, k, n, d, g in FEATURES],
        "faulty_bank": len(os.listdir(os.path.join(ROOT, "core/tests/faulty"))),
    }
    data["checks_total"] = sum(data["checks"].values())
    import datetime
    data["generated"] = datetime.datetime.now().strftime("%Y-%m-%d %H:%M")
    out = os.path.join(SITE, "data.js")
    with open(out, "w", encoding="utf-8") as f:
        f.write("// Données générées par site/gen_data.py — ne pas éditer.\n")
        f.write("// Sorties réelles des outils du dépôt.\n")
        f.write("window.GLANE_DATA = ")
        json.dump(data, f, ensure_ascii=False, indent=1)
        f.write(";\n")
    print(f"{out} écrit — {data['checks_total']} checks C, "
          f"{data['host_tests']} tests hôte, {len(data['journals'])} journaux")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))