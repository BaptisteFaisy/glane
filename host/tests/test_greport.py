#!/usr/bin/env python3
"""Glane — Copyright 2026 Baptiste Faisy. Licence Apache-2.0.
test_greport.py — rapport HTML autonome d'une campagne (idée C23)."""
import os
import sys
import tempfile
import unittest

HOST = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, HOST)
from greport import build_html, latency_histogram, quality_counts  # noqa: E402


def make_journal(path):
    rows = [
        {"s": "temp", "id": 1, "seq": 1, "ts_ns": 0, "d_ns": 100, "v_m": 10, "u": "C", "q": "ok", "f": "-"},
        {"s": "temp", "id": 1, "seq": 2, "ts_ns": 1000, "d_ns": 1200, "v_m": 11, "u": "C", "q": "ok", "f": "-"},
        {"s": "temp", "id": 1, "seq": 3, "ts_ns": 2000, "d_ns": 2500, "v_m": 12, "u": "C", "q": "stale", "f": "-"},
        {"s": "temp", "id": 1, "seq": 5, "ts_ns": 3000, "d_ns": 3100, "v_m": 13, "u": "C", "q": "ok", "f": "-"},
    ]
    import json
    with open(path, "w", encoding="utf-8") as f:
        for r in rows:
            f.write(json.dumps(r) + "\n")


class TestGreport(unittest.TestCase):
    def test_histogram(self):
        h = latency_histogram([1, 2, 3, 4, 5, 6, 7, 8, 9, 10], buckets=5)
        self.assertEqual(sum(c for _, c in h), 10)
        self.assertEqual(len(h), 5)

    def test_histogram_single_value(self):
        h = latency_histogram([7, 7, 7])
        self.assertEqual(sum(c for _, c in h), 3)

    def test_quality_counts(self):
        recs = [{"q": "ok"}, {"q": "ok"}, {"q": "stale"}]
        c = quality_counts(recs)
        self.assertEqual(c["ok"], 2)
        self.assertEqual(c["stale"], 1)

    def test_build_html_self_contained(self):
        with tempfile.TemporaryDirectory() as d:
            p = os.path.join(d, "j.jsonl")
            make_journal(p)
            html = build_html(p)
            self.assertIn("<!doctype html>", html)
            self.assertIn("Glane", html)
            self.assertIn("stale", html)         # qualité présente
            self.assertNotIn("http://", html)    # aucune dépendance externe
            self.assertNotIn("https://", html)
            self.assertNotIn("<script", html.lower())  # pas de JS


if __name__ == "__main__":
    unittest.main()