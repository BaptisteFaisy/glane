#!/usr/bin/env python3
"""
Glane — Copyright 2026 Baptiste Faisy. Licence Apache-2.0.

test_twin.py — test différentiel C ↔ Python de la calibration (GLA-05).

Compare glane_calib_apply (C, via tools/calib_dump) au jumeau Python
(host/glane_twin.py) sur 10 000 valeurs tirées dans les bornes du contrat,
graine fixe (reproductible). Tout désaccord est localisé (index, coefficients,
entrée, valeurs C et Python) — c'est la réduction du premier désaccord.
"""
import os
import random
import subprocess
import sys
import unittest

HOST = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
ROOT = os.path.dirname(HOST)
sys.path.insert(0, HOST)

from glane_twin import calib_apply, parse_coef  # noqa: E402

CALIB_DUMP = os.path.join(ROOT, "build", "calib_dump")
BOUND = 1048575  # CONTRACT.md §2.1


class TestCalibTwinUnit(unittest.TestCase):
    def test_known_vectors(self):
        # gain 1.0 (identité)
        self.assertEqual(calib_apply([0, 10 ** 6, 0, 0], 1, 12345), 12345)
        # x^2 : 10 -> 100
        self.assertEqual(calib_apply([0, 0, 10 ** 6, 0], 2, 10), 100)
        # arrondi demi à l'infini : 0.5 -> 1, -0.5 -> -1
        self.assertEqual(calib_apply([0, 500000, 0, 0], 1, 1), 1)
        self.assertEqual(calib_apply([0, -500000, 0, 0], 1, 1), -1)

    def test_parse_coef_twin(self):
        self.assertEqual(parse_coef("1.0"), 10 ** 6)
        self.assertEqual(parse_coef("-0.5"), -500000)
        self.assertEqual(parse_coef("125"), 125 * 10 ** 6)
        self.assertEqual(parse_coef("1e-3"), 1000)
        self.assertEqual(parse_coef("0.000001"), 1)


@unittest.skipUnless(os.path.exists(CALIB_DUMP),
                     "build/calib_dump absent — lancer `make all`")
class TestDifferential(unittest.TestCase):
    def test_10000_values(self):
        rnd = random.Random(20261112)  # graine fixe -> reproductible
        cases = []
        for _ in range(10000):
            deg = rnd.randint(0, 3)
            coef = [rnd.randint(-BOUND, BOUND) for _ in range(4)]
            x = rnd.randint(-BOUND, BOUND)
            cases.append((deg, coef, x))
        stdin = "\n".join(
            f"{deg} {c[0]} {c[1]} {c[2]} {c[3]} {x}" for deg, c, x in cases)
        proc = subprocess.run([CALIB_DUMP], input=stdin,
                              capture_output=True, text=True, timeout=120)
        self.assertEqual(proc.returncode, 0, proc.stderr)
        clines = proc.stdout.split()
        self.assertEqual(len(clines), len(cases),
                         "calib_dump n'a pas rendu une valeur par cas")
        first = None
        nbad = 0
        for i, (cl, (deg, coef, x)) in enumerate(zip(clines, cases)):
            cval = int(cl)
            pval = calib_apply(coef, deg, x)
            if cval != pval:
                nbad += 1
                if first is None:
                    first = (i, deg, coef, x, cval, pval)
        if first is not None:
            i, deg, coef, x, cval, pval = first
            self.fail(
                f"{nbad} divergence(s) C↔Python ; première à l'index {i} : "
                f"deg={deg} coef={coef} x={x} -> C={cval} Python={pval}")
        self.assertEqual(nbad, 0)


if __name__ == "__main__":
    unittest.main()