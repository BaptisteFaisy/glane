#!/usr/bin/env python3
"""Compte les lignes de texte (bandes non noires) d'une capture VGA.

Usage : png-rows.py <image.png> [ligne_debut] [ligne_fin]
Affiche le nombre de bandes de 16 px (lignes du mode texte VGA) non vides.
"""
import sys

from PIL import Image

img = Image.open(sys.argv[1]).convert("L")
w, h = img.size
y0 = int(sys.argv[2]) if len(sys.argv) > 2 else 0
y1 = int(sys.argv[3]) if len(sys.argv) > 3 else h
bandes = 0
for by in range(y0, min(y1, h), 16):
    crop = img.crop((0, by, w, min(by + 16, h)))
    px = crop.getdata()
    if any(v > 60 for v in px):
        bandes += 1
print(bandes)
