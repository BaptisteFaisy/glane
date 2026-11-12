#!/usr/bin/env python3
"""
Glane — Copyright 2026 Baptiste Faisy. Licence Apache-2.0.

gschema.py — validateur de conformité de l'arborescence publiée (GLA-08).

Comble le trou PUB-CONF : l'arborescence /dev/sensors/<name>/{...} est imposée
mais QNX ne livre aucun validateur. gschema vérifie une arborescence réelle
(sur la cible, ou un arbre de référence en test) : noms de capteurs valides,
feuilles obligatoires présentes, aucune feuille inconnue. Jumeau hôte de
core/src/schema.c — même contrat (CONTRACT.md §6). Rend un code non nul en cas
d'écart. 100 % stdlib.
"""
import os
import sys

REQUIRED_LEAVES = {"value", "quality", "seq", "ts_ns", "unit", "info"}
OPTIONAL_LEAVES = {"command"}
# Sous-objets valides de /dev/sensors/<name>/command/ (CONTRACT.md §6).
COMMAND_LEAVES = {"period", "calibrate", "inject", "selftest"}
NAME_MAX = 63
CHARSET = set("abcdefghijklmnopqrstuvwxyz0123456789._-")


def validate_name(name):
    """Jumeau de glane_schema_validate_name. Retourne None si valide, sinon
    un code d'erreur (chaîne)."""
    if not name:
        return "empty"
    if len(name) > NAME_MAX:
        return "too_long"
    if not all(c in CHARSET for c in name):
        return "bad_char"
    if name[0] == "." or name[-1] == ".":
        return "dot"
    return None


def validate_leaf(leaf):
    """Jumeau de glane_schema_validate_leaf."""
    if not leaf:
        return "empty"
    if len(leaf) > NAME_MAX:
        return "too_long"
    if not all(c in CHARSET for c in leaf):
        return "bad_char"
    if leaf not in REQUIRED_LEAVES and leaf not in OPTIONAL_LEAVES:
        return "unknown_leaf"
    return None


def validate_tree(root):
    """Valide une arborescence de capteurs. Retourne la liste des écarts
    (chaînes lisibles). Liste vide = conforme."""
    errors = []
    if not os.path.isdir(root):
        return [f"{root}: racine absente ou non-répertoire"]
    for name in sorted(os.listdir(root)):
        sensor_dir = os.path.join(root, name)
        if not os.path.isdir(sensor_dir):
            continue
        bad = validate_name(name)
        if bad:
            errors.append(f"capteur {name!r}: nom invalide ({bad})")
            continue
        leaves = set(os.listdir(sensor_dir))
        missing = REQUIRED_LEAVES - leaves
        if missing:
            errors.append(
                f"capteur {name!r}: feuilles obligatoires manquantes "
                f"{sorted(missing)}")
        for leaf in sorted(leaves):
            lp = os.path.join(sensor_dir, leaf)
            if os.path.isdir(lp):
                # Seul `command` est un sous-objet valide ; ses enfants sont
                # des commandes (period/calibrate/inject/selftest).
                if leaf != "command":
                    errors.append(
                        f"capteur {name!r}: sous-objet inattendu {leaf!r}")
                    continue
                for sub in sorted(os.listdir(lp)):
                    if sub not in COMMAND_LEAVES:
                        errors.append(
                            f"capteur {name!r}/command/{sub}: commande invalide")
                continue
            lb = validate_leaf(leaf)
            if lb:
                errors.append(f"capteur {name!r}: feuille {leaf!r} invalide ({lb})")
    return errors


def main(argv):
    if len(argv) < 2:
        print("usage: gschema.py <root>", file=sys.stderr)
        return 2
    errors = validate_tree(argv[1])
    if errors:
        print(f"SCHEMA FAIL ({len(errors)} écart(s)):", file=sys.stderr)
        for e in errors:
            print("  " + e, file=sys.stderr)
        return 1
    print("SCHEMA OK")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))