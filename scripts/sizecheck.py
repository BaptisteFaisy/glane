#!/usr/bin/env python3
"""
Glane — garde de taille (GLA-11).

Vérifie que tout fichier de code fait moins de 500 lignes et que toute
fonction fait moins de 50 lignes. Retourne un code non nul si une borne est
dépassée (utilisé par la CI et par `make sizecheck`).

Lignes de code = lignes physiques du fichier (commentaires et lignes vides
comptés, comme dans `wc -l`). Longueur d'une fonction = nombre de lignes
entre la ligne d'ouverture de son corps `{` et la ligne du `}` fermant,
mesurée par appariement d'accolades hors chaînes et commentaires.
"""
import os
import re
import sys

MAX_FILE_LINES = 500
MAX_FUNC_LINES = 50

CODE_EXT = {".c", ".h"}

# Mots-clés qui introduisent un bloc `{` sans être une définition de fonction.
NON_FUNC_KEYWORDS = {
    "if", "else", "for", "while", "do", "switch", "case", "default",
    "struct", "union", "enum", "typedef", "return",
}


def strip_line(line, in_block_comment):
    """Retire commentaires et chaînes d'une ligne pour l'appariement
    d'accolades. Retourne (ligne_nettoyée, in_block_comment)."""
    out = []
    i = 0
    n = len(line)
    in_str = False
    in_char = False
    while i < n:
        c = line[i]
        nxt = line[i + 1] if i + 1 < n else ""
        if in_block_comment:
            if c == "*" and nxt == "/":
                in_block_comment = False
                i += 2
                continue
            i += 1
            continue
        if in_str:
            if c == "\\":
                i += 2
                continue
            if c == '"':
                in_str = False
            i += 1
            continue
        if in_char:
            if c == "\\":
                i += 2
                continue
            if c == "'":
                in_char = False
            i += 1
            continue
        if c == "/" and nxt == "*":
            in_block_comment = True
            i += 2
            continue
        if c == "/" and nxt == "/":
            break
        if c == '"':
            in_str = True
            i += 1
            continue
        if c == "'":
            in_char = True
            i += 1
            continue
        out.append(c)
        i += 1
    return "".join(out), in_block_comment


def looks_like_func_open(prev_tokens):
    """Heuristique : la ligne qui précède `{` ressemble-t-elle à une
    définition de fonction (nom + parenthèses) plutôt qu'à un mot-clé ?"""
    t = prev_tokens.strip()
    if not t:
        return False
    # Se termine par `)` (signature de fonction) et commence par un
    # identifiant ou un type, pas par un mot-clé de contrôle.
    if not t.endswith(")"):
        return False
    first = re.match(r"[A-Za-z_][A-Za-z0-9_]*", t)
    if not first:
        return False
    if first.group(0) in NON_FUNC_KEYWORDS:
        return False
    return True


def check_file(path):
    errors = []
    with open(path, "r", encoding="utf-8", errors="replace") as f:
        lines = f.readlines()
    if len(lines) > MAX_FILE_LINES:
        errors.append(f"{path}: {len(lines)} lignes > {MAX_FILE_LINES}")

    # Appariement d'accolades pour mesurer la longueur des fonctions.
    depth = 0
    in_block = False
    func_start_line = 0
    func_depth_base = 0
    prev_clean = ""
    for idx, raw in enumerate(lines, start=1):
        clean, in_block = strip_line(raw, in_block)
        for ch in clean:
            if ch == "{":
                if depth == 0 and looks_like_func_open(prev_clean):
                    func_start_line = idx
                    func_depth_base = 0
                depth += 1
            elif ch == "}":
                depth -= 1
                if depth == 0 and func_start_line:
                    length = idx - func_start_line + 1
                    if length > MAX_FUNC_LINES:
                        errors.append(
                            f"{path}:{func_start_line}: fonction de "
                            f"{length} lignes > {MAX_FUNC_LINES}"
                        )
                    func_start_line = 0
        if clean.strip():
            prev_clean = clean
    return errors


def main():
    root = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    all_errors = []
    files = 0
    for dirpath, dirnames, filenames in os.walk(root):
        # Ignore build/, .git/, .github/
        dirnames[:] = [d for d in dirnames
                       if d not in {"build", ".git", ".github", "__pycache__"}]
        for fn in filenames:
            ext = os.path.splitext(fn)[1]
            if ext not in CODE_EXT:
                continue
            path = os.path.join(dirpath, fn)
            files += 1
            all_errors.extend(check_file(path))
    if all_errors:
        print(f"SIZECHECK FAIL ({len(all_errors)} problème(s)) :")
        for e in all_errors:
            print("  " + e)
        return 1
    print(f"SIZECHECK OK : {files} fichier(s) sous les bornes "
          f"(< {MAX_FILE_LINES} lignes/fichier, < {MAX_FUNC_LINES} lignes/fonction)")
    return 0


if __name__ == "__main__":
    sys.exit(main())