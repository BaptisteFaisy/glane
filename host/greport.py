#!/usr/bin/env python3
"""
Glane — Copyright 2026 Baptiste Faisy. Licence Apache-2.0.

greport.py — rapport HTML autonome d'une campagne de mesures (idée C23).

Transforme un journal de mesures en un fichier HTML unique (sans dépendance
externe, sans JS obligatoire) : répartition des qualités, histogramme de
latence, pertes de séquence, et un extrait du journal. C'est l'artefact
« preuve » joignable au dossier de soumission et lisible par un juge hors
ligne. 100 % stdlib.
"""
import html
import sys
from collections import Counter

from gjournal import read_journal, latencies_ns, seq_gaps
from glat import compute_stats


def quality_counts(records):
    return Counter(r.get("q", "?") for r in records)


def latency_histogram(latencies, buckets=10):
    """Histogramme simple : retourne [(borne_sup_ns, count), ...]."""
    if not latencies:
        return []
    lo, hi = min(latencies), max(latencies)
    if hi == lo:
        return [(hi, len(latencies))]
    width = (hi - lo) / buckets
    hist = [0] * buckets
    for v in latencies:
        idx = int((v - lo) / width)
        if idx >= buckets:
            idx = buckets - 1
        hist[idx] += 1
    return [(round(lo + width * (i + 1)), hist[i]) for i in range(buckets)]


def build_html(path, max_rows=200):
    records = read_journal(path)
    stats = compute_stats(latencies_ns(records))
    quals = quality_counts(records)
    gaps = seq_gaps(records)
    hist = latency_histogram(latencies_ns(records))

    def esc(x):
        return html.escape(str(x))

    qual_rows = "".join(f"<tr><td>{esc(k)}</td><td>{v}</td></tr>"
                        for k, v in sorted(quals.items()))
    stat_rows = "".join(f"<tr><td>{esc(k)}</td><td>{esc(v)}</td></tr>"
                        for k, v in stats.items())
    maxh = max((h[1] for h in hist), default=1) or 1
    bars = "".join(
        f"<div class=bar style='height:{int(120 * c / maxh)}px' "
        f"title='<= {esc(b)} ns : {c}'></div>"
        for b, c in hist)
    gap_txt = ", ".join(f"{esc(k)}={v}" for k, v in gaps.items()) or "aucune"
    rows = "".join(
        "<tr>" + "".join(f"<td>{esc(r.get(k, ''))}</td>"
                         for k in ("s", "seq", "ts_ns", "v_m", "u", "q", "f"))
        + "</tr>"
        for r in records[:max_rows])
    return f"""<!doctype html><html lang=fr><meta charset=utf-8>
<title>Glane — rapport de campagne</title>
<style>
body{{font-family:system-ui,sans-serif;margin:2rem;color:#222}}
h1{{font-size:1.4rem}} h2{{font-size:1.1rem;margin-top:1.5rem}}
table{{border-collapse:collapse;margin:.5rem 0}}
td,th{{border:1px solid #ccc;padding:.25rem .7rem;text-align:left;font-size:.9rem}}
.hist{{display:flex;align-items:flex-end;gap:2px;height:130px}}
.bar{{width:18px;background:#3b7ddd}}
</style>
<h1>Glane — rapport de campagne</h1>
<p>Journal : <code>{esc(path)}</code> · {len(records)} mesures</p>
<h2>Qualité des mesures</h2>
<table><tr><th>qualité</th><th>nombre</th></tr>{qual_rows}</table>
<h2>Latence source &rarr; livraison (ns)</h2>
<table><tr><th>mesure</th><th>valeur</th></tr>{stat_rows}</table>
<h2>Histogramme de latence</h2>
<div class=hist>{bars}</div>
<h2>Pertes de séquence</h2>
<p>{gap_txt}</p>
<h2>Extrait du journal ({min(max_rows, len(records))} premières lignes)</h2>
<table><tr><th>s</th><th>seq</th><th>ts_ns</th><th>v_m</th><th>u</th><th>q</th><th>f</th></tr>{rows}</table>
</html>"""


def main(argv):
    if len(argv) < 2:
        print("usage: greport.py <journal.jsonl> [out.html]", file=sys.stderr)
        return 2
    out = build_html(argv[1])
    if len(argv) > 2:
        with open(argv[2], "w", encoding="utf-8") as f:
            f.write(out)
        print(argv[2])
    else:
        print(out)
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))