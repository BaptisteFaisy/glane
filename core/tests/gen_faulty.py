#!/usr/bin/env python3
"""Génère la banque de 40 descriptions fautives (core/tests/faulty/) + 3
descriptions valides (core/tests/valid/) pour test_desc.c. Chaque fichier
fauteux correspond à un code d'erreur GLANE_DESC_* précis (voir test_desc.c).
"""
import json
import os

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
# ROOT = .../glane (le script est dans core/tests/)
FAULTY = os.path.join(ROOT, "core", "tests", "faulty")
VALID = os.path.join(ROOT, "core", "tests", "valid")

base = {
    "id": 1, "name": "temp-chassis", "driver": "tmp",
    "bus": {"kind": "i2c", "dev": "/dev/i2c0", "addr": "0x48"},
    "period_ms": 100, "unit": "C", "range": [-40.0, 125.0],
    "calib": {"kind": "poly", "coef": [0.0, 1.0]},
    "thresholds": {"warn": 85.0, "crit": 105.0},
    "deadline_ms": 150, "stale_after_ms": 500,
}


def mod(**kw):
    """Copie de base avec des champs remplacés ; '__del__' supprime une clé."""
    d = json.loads(json.dumps(base))
    for k, v in kw.items():
        if v == "__del__":
            d.pop(k, None)
        else:
            d[k] = v
    return json.dumps(d)


def mod_bus(**kw):
    d = json.loads(json.dumps(base))
    d["bus"].update(kw)
    return json.dumps(d)


def mod_calib(coef=None, kind=None):
    d = json.loads(json.dumps(base))
    if kind is not None:
        d["calib"]["kind"] = kind
    if coef is not None:
        d["calib"]["coef"] = coef
    return json.dumps(d)


def mod_thr(warn=None, crit=None):
    d = json.loads(json.dumps(base))
    if warn is not None:
        d["thresholds"]["warn"] = warn
    if crit is not None:
        d["thresholds"]["crit"] = crit
    return json.dumps(d)


# (nom de fichier, contenu). Codes attendus documentés dans test_desc.c.
files = {
    # --- EJSON : syntaxe mal formée (fichiers minimaux déclenchant l'erreur) ---
    "01_empty.json": "",
    "02_no_close_brace.json": '{"id": 1',
    "03_array_top.json": "[1, 2, 3]",
    "04_bad_escape.json": '{"name": "a\\qb"}',
    "05_unterminated_string.json": '{"name": "abc',
    "06_missing_colon.json": '{"id" 1}',
    "07_trailing_data.json": mod() + " oops",
    "08_range_missing_comma.json": '{"range": [-40.0 125.0]}',
    "09_range_three_elems.json": '{"range": [1.0, 2.0, 3.0]}',
    "10_coef_not_array.json": '{"calib": {"kind": "poly", "coef": 0.0}}',
    # --- ETYPE : type inattendu ---
    "11_id_string.json": '{"id": "x"}',
    "12_period_string.json": '{"period_ms": "100"}',
    "13_name_number.json": '{"name": 123}',
    "14_unit_number.json": '{"unit": 5}',
    "15_bus_kind_number.json": '{"bus": {"kind": 5}}',
    "16_addr_bad_hex.json": '{"bus": {"addr": "0xZZ"}}',
    "17_trailing_comma.json": '{"id": 1,}',
    "18_coef_string.json": '{"calib": {"kind": "poly", "coef": ["a"]}}',
    # --- EFIELD : champ obligatoire manquant / nom invalide ---
    "19_missing_id.json": mod(id="__del__"),
    "20_missing_name.json": mod(name="__del__"),
    "21_missing_driver.json": mod(driver="__del__"),
    "22_missing_bus.json": mod(bus="__del__"),
    "23_missing_period.json": mod(period_ms="__del__"),
    "24_missing_unit.json": mod(unit="__del__"),
    "25_missing_range.json": mod(range="__del__"),
    "26_missing_deadline.json": mod(deadline_ms="__del__"),
    "27_missing_stale.json": mod(stale_after_ms="__del__"),
    "28_name_uppercase.json": mod(name="Temp"),
    "29_name_empty.json": mod(name=""),
    "30_driver_space.json": mod(driver="tm p"),
    # --- ERANGE : valeur hors plage / incohérente ---
    "31_id_zero.json": mod(id=0),
    "32_period_zero.json": mod(period_ms=0),
    "33_stale_zero.json": mod(stale_after_ms=0),
    "34_range_inverted.json": mod(range=[125.0, -40.0]),
    "35_warn_gt_crit.json": mod_thr(warn=105.0, crit=85.0),
    "36_id_overflow.json": mod(id=99999999999),
    # --- EUNIT / EBUS ---
    "37_unit_unknown.json": mod(unit="parsec"),
    "38_bus_unknown.json": mod_bus(kind="can"),
    # --- ECALIB ---
    "39_calib_kind_unknown.json": mod_calib(kind="magic"),
    "40_coef_too_many.json": mod_calib(coef=[1.0, 2.0, 3.0, 4.0, 5.0]),
}

# Descriptions valides (cas positifs).
valid_files = {
    "temp-chassis.json": mod(),
    "sim-ramp.json": json.dumps({
        "id": 4, "name": "sim-ramp", "driver": "virtuel",
        "bus": {"kind": "virtuel"},
        "period_ms": 100, "unit": "C", "range": [-40.0, 125.0],
        "deadline_ms": 150, "stale_after_ms": 500,
    }),
    "bus-current.json": json.dumps({
        "id": 3, "name": "bus-current", "driver": "ina",
        "bus": {"kind": "i2c", "dev": "/dev/i2c1", "addr": "0x40"},
        "period_ms": 50, "unit": "mA", "range": [-3200.0, 3200.0],
        "calib": {"kind": "poly", "coef": [0.0, 1.0]},
        "thresholds": {"warn": 2000.0, "crit": 3000.0},
        "deadline_ms": 75, "stale_after_ms": 200,
    }),
}

os.makedirs(FAULTY, exist_ok=True)
os.makedirs(VALID, exist_ok=True)
for name, content in files.items():
    with open(os.path.join(FAULTY, name), "w", encoding="utf-8") as f:
        f.write(content)
for name, content in valid_files.items():
    with open(os.path.join(VALID, name), "w", encoding="utf-8") as f:
        f.write(content + "\n")

print(f"{len(files)} fichiers fautifs dans {FAULTY}")
print(f"{len(valid_files)} fichiers valides dans {VALID}")
assert len(files) == 40, "il faut exactement 40 cas fautifs"