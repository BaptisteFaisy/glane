#!/usr/bin/env python3
"""
Glane — Copyright 2026 Baptiste Faisy. Licence Apache-2.0.

gcanon.py — canonicalisation JSON + empreinte stable (idée C9).

Produit une forme canonique d'une description de capteur (clés triées,
séparateurs compacts, ASCII) et son empreinte SHA-256. Deux descriptions
sémantiquement identiques mais écrites différemment (ordre des clés, espaces)
ont la même empreinte : c'est la base du rejeu déterministe, du diff sémantique
et de la détection de changement de configuration. 100 % stdlib.
"""
import hashlib
import json
import sys


def canonical(obj):
    """Forme canonique d'un objet JSON déjà chargé : chaînes ASCII, clés
    triées, séparateurs compacts, sans espace superflu."""
    return json.dumps(obj, sort_keys=True, separators=(",", ":"),
                      ensure_ascii=True)


def canonical_text(text):
    """Canonise un texte JSON (le parse puis le recanonise)."""
    return canonical(json.loads(text))


def fingerprint(obj):
    """Empreinte SHA-256 (hex) de la forme canonique."""
    return hashlib.sha256(canonical(obj).encode("utf-8")).hexdigest()


def fingerprint_file(path):
    with open(path, "r", encoding="utf-8") as f:
        return fingerprint(json.load(f))


def main(argv):
    if len(argv) < 2:
        print("usage: gcanon.py <desc.json> [--canon|--finger]", file=sys.stderr)
        return 2
    path = argv[1]
    mode = argv[2] if len(argv) > 2 else "--finger"
    with open(path, "r", encoding="utf-8") as f:
        obj = json.load(f)
    if mode == "--canon":
        print(canonical(obj))
    else:
        print(fingerprint(obj))
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))