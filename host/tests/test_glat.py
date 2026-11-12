#!/usr/bin/env python3
"""Glane — Copyright 2026 Baptiste Faisy. Licence Apache-2.0.
test_glat.py — banc de latence : percentiles et pertes (GLA-09)."""
import os
import sys
import tempfile
import unittest

HOST = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, HOST)
from glat import analyze, compute_stats, percentile_nearest_rank  # noqa: E402


def write_journal(path, latencies, base_seq=1):
    with open(path, "w", encoding="utf-8") as f:
        for i, lat in enumerate(latencies):
            ts = i * 1000
            f.write('{"s":"t","id":1,"seq":%d,"ts_ns":%d,"d_ns":%d,'
                    '"v_m":1,"u":"C","q":"ok","f":"-"}\n'
                    % (base_seq + i, ts, ts + lat))


class TestPercentile(unittest.TestCase):
    def test_nearest_rank(self):
        s = list(range(1, 101))  # 1..100
        self.assertEqual(percentile_nearest_rank(s, 50), 50)
        self.assertEqual(percentile_nearest_rank(s, 99), 99)
        self.assertEqual(percentile_nearest_rank(s, 100), 100)
        self.assertEqual(percentile_nearest_rank([], 50), 0)

    def test_compute_stats(self):
        st = compute_stats([10, 20, 30])
        self.assertEqual(st["count"], 3)
        self.assertEqual(st["min_ns"], 10)
        self.assertEqual(st["max_ns"], 30)
        self.assertEqual(st["mean_ns"], 20.0)


class TestAnalyze(unittest.TestCase):
    def test_analyze_journal(self):
        with tempfile.TemporaryDirectory() as d:
            p = os.path.join(d, "j.jsonl")
            write_journal(p, [100, 200, 300, 400, 500])
            st = analyze(p)
            self.assertEqual(st["count"], 5)
            self.assertEqual(st["min_ns"], 100)
            self.assertEqual(st["max_ns"], 500)
            self.assertEqual(st["total_losses"], 0)

    def test_detects_seq_gap(self):
        with tempfile.TemporaryDirectory() as d:
            p = os.path.join(d, "j.jsonl")
            with open(p, "w", encoding="utf-8") as f:
                f.write('{"s":"t","seq":1,"ts_ns":0,"d_ns":10}\n')
                f.write('{"s":"t","seq":3,"ts_ns":1,"d_ns":11}\n')  # trou
            st = analyze(p)
            self.assertEqual(st["total_losses"], 1)
            self.assertEqual(st["seq_gaps"]["t"], 1)


if __name__ == "__main__":
    unittest.main()