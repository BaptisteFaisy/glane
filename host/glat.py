#!/usr/bin/env python3
"""
Glane — Copyright 2026 Baptiste Faisy. Licence Apache-2.0.

glat.py — banc de latence (GLA-09, idée C).

Lit un journal de mesures (CONTRACT.md §7, avec horodatage de livraison d_ns)
et calcule la latence source -> livraison : p50, p99, p99.9, max, min, moyenne
et nombre d'échantillons. Sortie JSON (machine) ou HTML (humain). Aucun
chiffre de latence n'est annoncé sans sortir d'ici : « mesurer plutôt
qu'affirmer ». 100 % stdlib.
"""
import json
import sys

from gjournal import read_journal, latencies_ns, seq_gaps


def percentile_nearest_rank(sorted_vals, p):
    """Percentile par rang le plus proche : plus petit élément dont le rang
    est >= ceil(p/100 * n). p dans (0, 100]."""
    n = len(sorted_vals)
    if n == 0:
        return 0
    import math
    rank = math.ceil(p / 100.0 * n)
    if rank < 1:
        rank = 1
    if rank > n:
        rank = n
    return sorted_vals[rank - 1]


def compute_stats(latencies):
    """Statistiques de latence (ns) à partir d'une liste brute."""
    if not latencies:
        return {"count": 0}
    s = sorted(latencies)
    n = len(s)
    return {
        "count": n,
        "min_ns": s[0],
        "max_ns": s[-1],
        "mean_ns": round(sum(s) / n, 1),
        "p50_ns": percentile_nearest_rank(s, 50),
        "p99_ns": percentile_nearest_rank(s, 99),
        "p99_9_ns": percentile_nearest_rank(s, 99.9),
    }


def analyze(path):
    """Analyse un journal : stats de latence + pertes de séquence."""
    records = read_journal(path)
    stats = compute_stats(latencies_ns(records))
    stats["seq_gaps"] = seq_gaps(records)
    stats["total_losses"] = sum(stats["seq_gaps"].values())
    return stats


def to_html(stats):
    rows = "".join(
        f"<tr><td>{k}</td><td>{v}</td></tr>"
        for k, v in stats.items()
        if not isinstance(v, dict)
    )
    gaps = stats.get("seq_gaps", {})
    gap_rows = "".join(f"<tr><td>{k}</td><td>{v}</td></tr>"
                       for k, v in gaps.items()) or "<tr><td colspan=2>aucune perte</td></tr>"
    return (
        "<!doctype html><html lang=fr><meta charset=utf-8>"
        "<title>Glane — banc de latence</title>"
        "<style>body{font-family:system-ui,sans-serif;margin:2rem}"
        "table{border-collapse:collapse}td,th{border:1px solid #ccc;"
        "padding:.3rem .8rem;text-align:left}h1{font-size:1.3rem}</style>"
        "<h1>Glane — banc de latence (glat)</h1>"
        "<h2>Latence source &rarr; livraison</h2>"
        f"<table><tr><th>mesure</th><th>valeur</th></tr>{rows}</table>"
        "<h2>Pertes de séquence par capteur</h2>"
        f"<table><tr><th>capteur</th><th>trous</th></tr>{gap_rows}</table>"
        "</html>"
    )


def main(argv):
    if len(argv) < 2:
        print("usage: glat.py <journal.jsonl> [--json|--html]", file=sys.stderr)
        return 2
    path = argv[1]
    mode = argv[2] if len(argv) > 2 else "--json"
    stats = analyze(path)
    if mode == "--html":
        print(to_html(stats))
    else:
        print(json.dumps(stats, indent=2, sort_keys=True))
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))