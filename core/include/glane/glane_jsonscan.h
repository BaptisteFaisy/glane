/*
 * Glane — Copyright 2026 Baptiste Faisy. Licence Apache-2.0.
 *
 * glane_jsonscan.h — micro-scanneur JSON interne (GLA-02).
 *
 * Parseur JSON minimal, pur (aucune allocation, aucune E/S), utilisé par
 * desc.c pour analyser les descriptions de capteur. Travaille sur un tampon
 * + longueur et suit les numéros de ligne pour les messages d'erreur. Ce
 * n'est pas une bibliothèque JSON publique : l'API est interne au socle.
 */
#ifndef GLANE_JSONSCAN_H
#define GLANE_JSONSCAN_H

#include <stddef.h>

#include "glane/glane_desc.h"  /* glane_desc_error + codes GLANE_DESC_* */

#ifdef __cplusplus
extern "C" {
#endif

/* Curseur sur le tampon JSON. `line` est 1-based, incrémenté à chaque '\n'. */
typedef struct {
    const char *s;
    size_t len;
    size_t pos;
    int line;
} json_cursor;

/* Callback appelé par json_parse_object pour chaque membre clé:valeur.
 * Retourne 0 si OK, sinon le code d'erreur (et remplit err).              */
typedef int (*json_member_fn)(void *ctx, const char *key, json_cursor *c,
                              glane_desc_error *err);

/* Remplit `err` (code, champ, ligne, cause) et retourne `code`. Utilisé par
 * tout le parseur pour signaler une erreur de façon uniforme.            */
int json_fail(glane_desc_error *err, int code, const char *field,
              int line, const char *cause);

void json_ws(json_cursor *c);                 /* saute blancs, suit les lignes */
int  json_ch(json_cursor *c);                 /* caractère courant ou -1 (fin) */
int  json_expect(json_cursor *c, char ch, glane_desc_error *err, const char *f);
int  json_string(json_cursor *c, char *out, size_t outmax,
                 glane_desc_error *err, const char *f);
int  json_number_token(json_cursor *c, char *buf, size_t bufmax,
                       glane_desc_error *err, const char *f);
int  json_skip_value(json_cursor *c, glane_desc_error *err, const char *f);
int  json_parse_object(json_cursor *c, void *ctx, json_member_fn fn,
                       glane_desc_error *err, const char *obj);

#ifdef __cplusplus
}
#endif

#endif /* GLANE_JSONSCAN_H */