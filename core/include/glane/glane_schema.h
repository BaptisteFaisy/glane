/*
 * Glane — Copyright 2026 Baptiste Faisy. Licence Apache-2.0.
 *
 * glane_schema.h — validation des noms et des objets publiés (GLA-08,
 * §5.3 du cahier des charges).
 *
 * L'arborescence publiée est imposée : /dev/sensors/<name>/{...}. Le C valide
 * les NOMS et la liste des feuilles requises ; le jumeau hôte `gschema`
 * (Python) valide une arborescence réelle sur disque. Les deux partagent ce
 * même contrat, codé ici.
 */
#ifndef GLANE_SCHEMA_H
#define GLANE_SCHEMA_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Feuilles obligatoires d'un objet capteur (ordre canonique).            */
#define GLANE_LEAF_VALUE   "value"
#define GLANE_LEAF_QUALITY "quality"
#define GLANE_LEAF_SEQ     "seq"
#define GLANE_LEAF_TS_NS   "ts_ns"
#define GLANE_LEAF_UNIT    "unit"
#define GLANE_LEAF_INFO    "info"

/* Nombre de feuilles obligatoires. */
#define GLANE_LEAF_REQUIRED 6

/* Codes de retour de glane_schema_validate_name / _leaf. */
enum {
    GLANE_SCHEMA_OK         = 0,
    GLANE_SCHEMA_EMPTY      = 1,  /* nom vide                              */
    GLANE_SCHEMA_TOO_LONG   = 2,  /* dépasse GLANE_NAME_MAX - 1             */
    GLANE_SCHEMA_BAD_CHAR   = 3,  /* caractère hors [a-z0-9._-]            */
    GLANE_SCHEMA_DOT        = 4,  /* commence ou finit par '.'             */
};

/* Valide un nom de capteur : non vide, <= 63 octets, caractères
 * [a-z0-9._-], sans '.' initial ou final. Retourne un code GLANE_SCHEMA_*.
 * `len` peut valoir SIZE_MAX si `name` est NUL-terminé (sinon exact).     */
int glane_schema_validate_name(const char *name, size_t len);

/* Valide un nom de feuille d'objet publié. Accepte la liste canonique
 * (value/quality/seq/ts_ns/unit/info) plus `command`. Même charset.       */
int glane_schema_validate_leaf(const char *leaf, size_t len);

/* Vrai (1) si `leaf` fait partie des feuilles obligatoires. */
int glane_schema_is_required_leaf(const char *leaf, size_t len);

/* Message court par code GLANE_SCHEMA_* (statique, jamais NULL). */
const char *glane_schema_errstr(int code);

#ifdef __cplusplus
}
#endif

#endif /* GLANE_SCHEMA_H */