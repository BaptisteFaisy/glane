#!/usr/bin/env python3
"""
Glane — Copyright 2026 Baptiste Faisy. Licence Apache-2.0.

glane_twin.py — jumeau Python de la calibration entière (GLA-05).

Réimplémente glane_calib_apply (core/src/calib.c) en Python pur, avec des
entiers de précision arbitraire. Le test différentiel (host/tests/
test_differential.py) compare ce jumeau au C sur 10 000 valeurs : tout écart
trahit une divergence d'arrondi ou de débordement. C'est la preuve que le
rejeu de la calibration est bit à bit.
"""

SCALE = 10 ** 6           # GLANE_CALIB_SCALE
INT32_MAX = 2 ** 31 - 1
INT32_MIN = -(2 ** 31)


def calib_apply(coef, deg, x):
    """Polynôme de calibration, identique à glane_calib_apply.

    coef : liste d'au moins deg+1 entiers échelle 1e6 (a0..a3).
    deg  : degré effectif (0..3).
    x    : entrée brute en milli-unités (int).
    Retourne la valeur calibrée en milli-unités (int32), arrondi au plus
    proche (demi à l'infini) puis clampé dans int32.
    """
    if deg < 0:
        deg = 0
    if deg > 3:
        deg = 3
    # Horner : acc = coef[deg]; puis acc = acc*x + coef[i] (i décroissant).
    acc = coef[deg]
    for i in range(deg - 1, -1, -1):
        acc = acc * x + coef[i]
    # round_div_clamp : demi à l'infini, division tronquée vers zéro.
    half = SCALE // 2
    num = acc + half if acc >= 0 else acc - half
    q = abs(num) // SCALE
    if num < 0:
        q = -q
    if q > INT32_MAX:
        q = INT32_MAX
    if q < INT32_MIN:
        q = INT32_MIN
    return q


def parse_coef(text):
    """Jumeau de glane_calib_parse_coef : décimal signé (exposant optionnel)
    -> entier échelle 1e6. Utilisé pour vérifier le parseur C de coefficients.
    """
    s = text.strip()
    neg = False
    if s[0:1] in ("+", "-"):
        neg = s[0] == "-"
        s = s[1:]
    exp = 0
    if "e" in s or "E" in s:
        idx = s.lower().index("e")
        exp = int(s[idx + 1:])
        s = s[:idx]
    if "." in s:
        int_part, frac_part = s.split(".", 1)
    else:
        int_part, frac_part = s, ""
    digits = int_part + frac_part
    if not digits.isdigit():
        raise ValueError(f"coef invalide: {text!r}")
    sig = int(digits)
    fd = len(frac_part)
    total = exp - fd + 6
    if total >= 0:
        scaled = sig * (10 ** total)
    else:
        d = 10 ** (-total)
        scaled = (sig + d // 2) // d
    return -scaled if neg else scaled