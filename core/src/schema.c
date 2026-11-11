/*
 * Glane — Copyright 2026 Baptiste Faisy. Licence Apache-2.0.
 *
 * schema.c — validation des noms et des objets publiés (GLA-08, §5.3).
 *
 * L'arborescence /dev/sensors/<name>/{...} est imposée. Ce module valide les
 * NOMS (capteur et feuilles) avant publication ; le jumeau hôte `gschema`
 * (Python, L5) valide une arborescence réelle sur disque. Même contrat.
 */
#include "glane/glane_schema.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#define GLANE_NAME_MAX_LOCAL 64  /* = GLANE_NAME_MAX de glane_desc.h */

/* Feuilles obligatoires (ordre canonique, cf. glane_schema.h). */
static const char *const required_leaves[GLANE_LEAF_REQUIRED] = {
    GLANE_LEAF_VALUE, GLANE_LEAF_QUALITY, GLANE_LEAF_SEQ,
    GLANE_LEAF_TS_NS, GLANE_LEAF_UNIT, GLANE_LEAF_INFO,
};

/* `command` est une feuille optionnelle mais valide. */
static const char *const optional_leaf = "command";

static int charset_ok(const char *s, size_t len)
{
    for (size_t i = 0; i < len; i++) {
        char c = s[i];
        int ok = (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') ||
                 c == '.' || c == '_' || c == '-';
        if (!ok) {
            return 0;
        }
    }
    return 1;
}

/* Résout la longueur effective (SIZE_MAX => NUL-terminé). */
static size_t resolve_len(const char *s, size_t len)
{
    if (s == NULL) {
        return 0;
    }
    if (len == SIZE_MAX) {
        return strlen(s);
    }
    return len;
}

int glane_schema_validate_name(const char *name, size_t len)
{
    size_t n = resolve_len(name, len);
    if (name == NULL || n == 0) {
        return GLANE_SCHEMA_EMPTY;
    }
    if (n >= GLANE_NAME_MAX_LOCAL) {
        return GLANE_SCHEMA_TOO_LONG;
    }
    if (!charset_ok(name, n)) {
        return GLANE_SCHEMA_BAD_CHAR;
    }
    if (name[0] == '.' || name[n - 1] == '.') {
        return GLANE_SCHEMA_DOT;
    }
    return GLANE_SCHEMA_OK;
}

int glane_schema_validate_leaf(const char *leaf, size_t len)
{
    size_t n = resolve_len(leaf, len);
    if (leaf == NULL || n == 0) {
        return GLANE_SCHEMA_EMPTY;
    }
    if (n >= GLANE_NAME_MAX_LOCAL) {
        return GLANE_SCHEMA_TOO_LONG;
    }
    if (!charset_ok(leaf, n)) {
        return GLANE_SCHEMA_BAD_CHAR;
    }
    /* Doit être une feuille connue (obligatoire ou `command`). */
    for (int i = 0; i < GLANE_LEAF_REQUIRED; i++) {
        if (strlen(required_leaves[i]) == n &&
            strncmp(required_leaves[i], leaf, n) == 0) {
            return GLANE_SCHEMA_OK;
        }
    }
    if (strlen(optional_leaf) == n &&
        strncmp(optional_leaf, leaf, n) == 0) {
        return GLANE_SCHEMA_OK;
    }
    return GLANE_SCHEMA_BAD_CHAR;  /* feuille inconnue */
}

int glane_schema_is_required_leaf(const char *leaf, size_t len)
{
    size_t n = resolve_len(leaf, len);
    if (leaf == NULL || n == 0) {
        return 0;
    }
    for (int i = 0; i < GLANE_LEAF_REQUIRED; i++) {
        if (strlen(required_leaves[i]) == n &&
            strncmp(required_leaves[i], leaf, n) == 0) {
            return 1;
        }
    }
    return 0;
}

const char *glane_schema_errstr(int code)
{
    switch (code) {
    case GLANE_SCHEMA_OK:       return "ok";
    case GLANE_SCHEMA_EMPTY:    return "nom vide";
    case GLANE_SCHEMA_TOO_LONG: return "nom trop long (>= 64 octets)";
    case GLANE_SCHEMA_BAD_CHAR: return "caractère invalide ou feuille inconnue";
    case GLANE_SCHEMA_DOT:      return "commence ou finit par '.'";
    default:                    return "erreur inconnue";
    }
}