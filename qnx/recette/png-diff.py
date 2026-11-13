#!/usr/bin/env python3
"""Compare deux captures VGA et liste les zones (bandes de texte) qui changent."""
import sys

from PIL import Image, ImageChops

a = Image.open(sys.argv[1]).convert("L")
b = Image.open(sys.argv[2]).convert("L")
diff = ImageChops.difference(a, b)
bbox = diff.getbbox()
print("boite de difference :", bbox)
if bbox:
    x0, y0, x1, y1 = bbox
    for y in range(y0, y1):
        ligne = "".join("#" if diff.getpixel((x, y)) > 40 else "." for x in range(x0, min(x1, x0 + 120)))
        print("y=%3d %s" % (y, ligne))
