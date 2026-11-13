#!/usr/bin/env python3
"""Cherche un motif texte dans une capture VGA, ligne par ligne (mode texte
VGA : 16 px de haut). Evite les problemes de segmentation de tesseract sur
la page entiere (lignes de ====).

Usage : png-find.py <image.png> <motif> [facteur]
Sortie : l'index de la premiere ligne qui contient le motif (-1 sinon).
"""
import re
import subprocess
import sys
import tempfile

from PIL import Image

img = Image.open(sys.argv[1]).convert("L")
w, h = img.size
motif = re.compile(sys.argv[2], re.I)
fact = float(sys.argv[3]) if len(sys.argv) > 3 else 4.0
for i, y in enumerate(range(0, h, 16)):
    crop = img.crop((0, y, w, min(y + 16, h))).resize(
        (int(w * fact), int(min(16, h - y) * fact)), Image.LANCZOS)
    with tempfile.NamedTemporaryFile("w", suffix=".png", delete=False) as fh:
        chemin = fh.name
    crop.save(chemin)
    out = subprocess.run(["tesseract", chemin, "-"], capture_output=True, text=True)
    if motif.search(out.stdout):
        print(i)
        print(out.stdout.strip()[:120])
        sys.exit(0)
print(-1)
sys.exit(1)
