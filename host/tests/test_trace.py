#!/usr/bin/env python3
"""Glane — Copyright 2026 Baptiste Faisy. Licence Apache-2.0.
test_trace.py — matrice de traçabilité des exigences GLA (idée C8)."""
import os
import sys
import unittest

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
sys.path.insert(0, os.path.join(ROOT, "scripts"))
import glane_trace  # noqa: E402


class TestTrace(unittest.TestCase):
    def test_requirements_loaded(self):
        reqs = glane_trace.load_requirements()
        ids = [r[0] for r in reqs]
        self.assertIn("GLA-01", ids)
        self.assertIn("GLA-24", ids)
        self.assertEqual(len(reqs), 24)

    def test_references_scanned(self):
        refs = glane_trace.scan_references()
        # GLA-05 (calibration) est citée dans calib.c et test_calib.c
        self.assertIn("GLA-05", refs)
        self.assertTrue(any("calib" in f for f in refs["GLA-05"]))

    def test_matrix_coverage(self):
        rows = glane_trace.build_matrix()
        by_id = {r["id"]: r for r in rows}
        self.assertTrue(by_id["GLA-05"]["covered"])   # test_calib
        self.assertTrue(by_id["GLA-02"]["covered"])   # test_desc
        self.assertTrue(by_id["GLA-08"]["covered"])   # test_schema
        self.assertTrue(by_id["GLA-04"]["covered"])   # test_virtuel/test_sim

    def test_markdown_output(self):
        md = glane_trace.to_markdown(glane_trace.build_matrix())
        self.assertIn("# Glane — matrice de traçabilité", md)
        self.assertIn("| GLA-05 |", md)
        self.assertIn("exigences citées dans un test", md)


if __name__ == "__main__":
    unittest.main()