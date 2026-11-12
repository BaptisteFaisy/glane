#!/usr/bin/env python3
"""
Glane — Copyright 2026 Baptiste Faisy. Licence Apache-2.0.

test_sim.py — publisher virtuel hôte : rejeu déterministe (GLA-04) et
intégration desc -> virtuel -> calib -> journal.
"""
import json
import os
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path

HOST = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
ROOT = os.path.dirname(HOST)
sys.path.insert(0, HOST)

SIM = os.path.join(ROOT, "build", "glaned-sim")
DESC = os.path.join(ROOT, "core", "tests", "valid", "temp-chassis.json")


@unittest.skipUnless(os.path.exists(SIM), "build/glaned-sim absent — `make all`")
class TestSim(unittest.TestCase):
    def _run(self, out, seed=42, wf="sine", steps=20, lat=1000000):
        return subprocess.run(
            [SIM, DESC, str(steps), wf, str(seed), str(lat), out],
            capture_output=True, text=True, timeout=30)

    def test_journal_produced_and_valid(self):
        with tempfile.TemporaryDirectory() as d:
            out = os.path.join(d, "j.jsonl")
            r = self._run(out)
            self.assertEqual(r.returncode, 0, r.stderr)
            lines = Path(out).read_text(encoding="utf-8").splitlines()
            self.assertEqual(len(lines), 20)
            rec = json.loads(lines[0])
            for key in ("s", "id", "seq", "ts_ns", "d_ns", "v_m", "u", "q", "f"):
                self.assertIn(key, rec)
            self.assertEqual(rec["s"], "temp-chassis")
            self.assertEqual(rec["seq"], 1)
            self.assertIn("simulated", rec["f"])

    def test_deterministic_replay(self):
        """Deux exécutions mêmes paramètres -> journaux identiques octet à octet."""
        with tempfile.TemporaryDirectory() as d:
            a = os.path.join(d, "a.jsonl")
            b = os.path.join(d, "b.jsonl")
            self._run(a, seed=7, wf="noise", steps=50)
            self._run(b, seed=7, wf="noise", steps=50)
            self.assertEqual(Path(a).read_bytes(), Path(b).read_bytes())

    def test_seed_changes_output(self):
        with tempfile.TemporaryDirectory() as d:
            a = os.path.join(d, "a.jsonl")
            b = os.path.join(d, "b.jsonl")
            self._run(a, seed=1, wf="noise", steps=50)
            self._run(b, seed=2, wf="noise", steps=50)
            self.assertNotEqual(Path(a).read_bytes(), Path(b).read_bytes())

    def test_invalid_desc_rejected(self):
        with tempfile.TemporaryDirectory() as d:
            bad = os.path.join(d, "bad.json")
            Path(bad).write_text('{"id": 0}', encoding="utf-8")
            out = os.path.join(d, "j.jsonl")
            r = subprocess.run([SIM, bad, "5", "const", "1", "0", out],
                               capture_output=True, text=True, timeout=30)
            self.assertNotEqual(r.returncode, 0)


if __name__ == "__main__":
    unittest.main()