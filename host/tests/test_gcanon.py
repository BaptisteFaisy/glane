#!/usr/bin/env python3
"""Glane — Copyright 2026 Baptiste Faisy. Licence Apache-2.0.
test_gcanon.py — canonicalisation JSON + empreinte stable (idée C9)."""
import os
import sys
import unittest

HOST = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, HOST)
from gcanon import canonical, canonical_text, fingerprint  # noqa: E402


class TestGcanon(unittest.TestCase):
    def test_key_order_irrelevant(self):
        a = {"id": 1, "name": "x", "period_ms": 100}
        b = {"period_ms": 100, "name": "x", "id": 1}
        self.assertEqual(canonical(a), canonical(b))
        self.assertEqual(fingerprint(a), fingerprint(b))

    def test_whitespace_irrelevant(self):
        self.assertEqual(canonical_text('{ "a" : 1 , "b" : 2 }'),
                         canonical_text('{"b":2,"a":1}'))

    def test_different_content_different_fingerprint(self):
        self.assertNotEqual(fingerprint({"a": 1}), fingerprint({"a": 2}))

    def test_canonical_is_compact_sorted(self):
        self.assertEqual(canonical({"b": 1, "a": 2}), '{"a":2,"b":1}')

    def test_fingerprint_is_sha256_hex(self):
        fp = fingerprint({"x": 1})
        self.assertEqual(len(fp), 64)
        int(fp, 16)  # ne doit pas lever


if __name__ == "__main__":
    unittest.main()