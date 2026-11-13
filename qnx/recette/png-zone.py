#!/usr/bin/env python3
"""Recadre une zone d'une capture VGA (lignes) et la grossit pour l'OCR.

Usage : png-zone.py <src.png> <dst.png> <ligne_debut> <ligne_fin> [facteur]
"""
import sys

from PIL import Image

src, dst = sys.argv[1], sys.argv[2]
y0, y1 = int(sys.argv[3]), int(sys.argv[4])
fact = float(sys.argv[5]) if len(sys.argv) > 5 else 2.0
im = Image.open(src).convert("L")
w, h = im.size
im.crop((0, y0, w, min(y1, h))).resize((int(w * fact), int((min(y1, h) - y0) * fact)),
                                        Image.LANCZOS).save(dst)
print("zone %d-%d -> %s" % (y0, y1, dst))
