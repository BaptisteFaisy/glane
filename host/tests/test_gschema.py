#!/usr/bin/env python3
"""Glane — Copyright 2026 Baptiste Faisy. Licence Apache-2.0.
test_gschema.py — validateur d'arborescence publiée (GLA-08)."""
import os
import sys
import tempfile
import unittest
from pathlib import Path

HOST = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, HOST)
from gschema import (validate_name, validate_leaf, validate_tree,  # noqa: E402
                     REQUIRED_LEAVES)


def make_sensor(root, name, leaves, subdirs=None):
    d = os.path.join(root, name)
    os.makedirs(d, exist_ok=True)
    for lf in leaves:
        Path(os.path.join(d, lf)).write_text("x", encoding="utf-8")
    for sd, sub_leaves in (subdirs or {}).items():
        sdpath = os.path.join(d, sd)
        os.makedirs(sdpath, exist_ok=True)
        for sl in sub_leaves:
            Path(os.path.join(sdpath, sl)).write_text("x", encoding="utf-8")
    return d


class TestNames(unittest.TestCase):
    def test_valid_names(self):
        for n in ("temp-chassis", "imu_0", "a", "x_y-z.0"):
            self.assertIsNone(validate_name(n), n)

    def test_invalid_names(self):
        self.assertEqual(validate_name(""), "empty")
        self.assertEqual(validate_name("a" * 64), "too_long")
        self.assertEqual(validate_name("Temp"), "bad_char")
        self.assertEqual(validate_name(".x"), "dot")
        self.assertEqual(validate_name("x."), "dot")

    def test_leaves(self):
        for lf in REQUIRED_LEAVES:
            self.assertIsNone(validate_leaf(lf), lf)
        self.assertIsNone(validate_leaf("command"))
        self.assertEqual(validate_leaf("unknown"), "unknown_leaf")
        self.assertEqual(validate_leaf("Value"), "bad_char")


class TestTree(unittest.TestCase):
    def test_valid_tree(self):
        with tempfile.TemporaryDirectory() as root:
            make_sensor(root, "temp-chassis", sorted(REQUIRED_LEAVES),
                        subdirs={"command": ["period", "inject"]})
            make_sensor(root, "imu_0", sorted(REQUIRED_LEAVES))
            self.assertEqual(validate_tree(root), [])

    def test_missing_leaf(self):
        with tempfile.TemporaryDirectory() as root:
            leaves = sorted(REQUIRED_LEAVES - {"quality"})
            make_sensor(root, "temp", leaves)
            errs = validate_tree(root)
            self.assertTrue(any("quality" in e for e in errs), errs)

    def test_bad_sensor_name(self):
        with tempfile.TemporaryDirectory() as root:
            make_sensor(root, "BadName", sorted(REQUIRED_LEAVES))
            errs = validate_tree(root)
            self.assertTrue(any("invalide" in e for e in errs), errs)

    def test_unknown_leaf(self):
        with tempfile.TemporaryDirectory() as root:
            make_sensor(root, "temp", sorted(REQUIRED_LEAVES | {"bogus"}))
            errs = validate_tree(root)
            self.assertTrue(any("bogus" in e for e in errs), errs)

    def test_missing_root(self):
        errs = validate_tree("/nonexistent/path/xyz")
        self.assertEqual(len(errs), 1)


if __name__ == "__main__":
    unittest.main()