#!/usr/bin/env python3
"""Sync les chiffres de latence cités dans les docs avec la capture du jour.

Les chiffres publiés ne sont jamais écrits à la main : ils sortent de la
capture réelle (`qnx/capture/runtime-console.txt`, produite par
`qnx/run-glane-qemu.sh` sur QNX 8.0). Ce script recopie la ligne
`glane-latence:` de la capture dans les documents qui la citent, pour que
la soumission ne puisse pas annoncer une latence que la cible ne produit
plus.

Usage : python3 qnx/sync_numbers.py
"""
import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
CAP = os.path.join(ROOT, "qnx/capture/runtime-console.txt")

lines = [l for l in open(CAP, encoding="utf-8", errors="replace").read().splitlines()
         if l.startswith("glane-latence:")]
rep = []
for l in lines:
    m = re.search(r"(\d+) publications, p50=(\d+) ns, p99=(\d+) ns, p99\.9=(\d+) ns, max=(\d+) ns", l)
    if m:
        rep.append(dict(n=int(m.group(1)), p50=int(m.group(2)), p99=int(m.group(3)),
                        p999=int(m.group(4)), mx=int(m.group(5))))
if not rep:
    sys.exit("aucune ligne glane-latence exploitable")
main = max(rep, key=lambda r: r["n"])
p50_min = min(r["p50"] for r in rep)
p50_max = max(r["p50"] for r in rep)
p99_max = max(r["p99"] for r in rep)
print("runs :", rep)
print("retenu :", main, "| p50", p50_min, "-", p50_max, "| p99 max", p99_max)


def fr(n):
    """Millisecondes en nombre lisible : 1 234 567 (espaces fines)."""
    return "{:,}".format(n).replace(",", " ")


def patch(rel, substitutions):
    """Applique des substitutions (motif, remplacement, nb_attendu)."""
    p = os.path.join(ROOT, rel)
    s = open(p, encoding="utf-8").read()
    for motif, rempl, attendu in substitutions:
        s2, n = re.subn(motif, rempl, s)
        if n != attendu:
            sys.exit("%s : %d remplacements (attendu %d) pour %r" % (rel, n, attendu, motif))
        s = s2
    open(p, "w", encoding="utf-8").write(s)
    print("patché :", rel)


# --- README.en.md : la ligne de latence du tableau
lat = "p50=%d ns, p99=%d ns, p99.9=%d ns, max=%d ns" % (
    main["p50"], main["p99"], main["p999"], main["mx"])
patch("README.en.md", [
    (r"p50=\d+ ns, p99=\d+ ns, p99\.9=\d+ ns, max=\d+ ns", lat, 1),
])

# --- DEVPOST.md : quatre passages (le texte, le tableau, la plage, le résumé)
# Le passage « publication latency of p50 = … » est en gras, celui du tableau
# ne l'est pas : les deux variantes sont couvertes.
patch("docs/DEVPOST.md", [
    (r"p50 stayed between \*\*[\d ]+ ns and [\d ]+ ns\*\*, and\n  the worst p99 reached \*\*[\d ]+ ns\*\*",
     "p50 stayed between **%s ns and %s ns**, and\n  the worst p99 reached **%s ns**"
     % (fr(p50_min), fr(p50_max), fr(p99_max)), 1),
    (r"p50 = \*\*[\d ]+ ns\*\*, p99 = \*\*[\d ]+ ns\*\*",
     "p50 = **%s ns**, p99 = **%s ns**" % (fr(main["p50"]), fr(main["p99"])), 1),
    (r"p50 = [\d ]+ ns, p99 = [\d ]+ ns",
     "p50 = %s ns, p99 = %s ns" % (fr(main["p50"]), fr(main["p99"])), 1),
    (r"p50 range [\d ]+–[\d ]+ ns, p99 up to [\d ]+ ns",
     "p50 range %s–%s ns, p99 up to %s ns" % (fr(p50_min), fr(p50_max), fr(p99_max)), 1),
])

# --- VIDEO.md : la ligne citée par le script de la vidéo (2 occurrences)
patch("docs/VIDEO.md", [
    (r"p50=\d+ ns, p99=\d+ ns, p99\.9=\d+ ns, max=\d+ ns", lat, 2),
])
