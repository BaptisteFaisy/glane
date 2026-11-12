#!/usr/bin/env python3
"""
Glane — Copyright 2026 Baptiste Faisy. Licence Apache-2.0.

gjournals.py — lecteur partagé du journal de mesures (CONTRACT.md §7).
Une ligne = une mesure JSON ASCII. Utilisé par glat et greport (zéro
duplication de la logique de lecture).
"""
import json


def read_journal(path):
    """Lit un journal .jsonl et retourne la liste des mesures (dicts).

    Chaque ligne doit être un objet JSON. Les lignes vides sont ignorées.
    Lève ValueError si une ligne n'est pas du JSON valide.
    """
    out = []
    with open(path, "r", encoding="utf-8") as f:
        for lineno, line in enumerate(f, start=1):
            line = line.strip()
            if not line:
                continue
            try:
                out.append(json.loads(line))
            except json.JSONDecodeError as e:
                raise ValueError(f"ligne {lineno}: JSON invalide ({e})") from e
    return out


def latencies_ns(records):
    """Retourne la liste des latences (d_ns - ts_ns) en ns, pour les mesures
    qui portent un horodatage de livraison."""
    lat = []
    for r in records:
        if "d_ns" in r and "ts_ns" in r:
            lat.append(int(r["d_ns"]) - int(r["ts_ns"]))
    return lat


def seq_gaps(records):
    """Compte les trous de séquence par capteur (pertes). Retourne un dict
    {sensor_name: nb_trous}."""
    last = {}
    gaps = {}
    for r in records:
        s = r.get("s", "?")
        seq = int(r.get("seq", 0))
        if s in last:
            expected = (last[s] + 1) & 0xFFFFFFFF
            if seq != expected:
                gaps[s] = gaps.get(s, 0) + 1
        last[s] = seq
    return gaps