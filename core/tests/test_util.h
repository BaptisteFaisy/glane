/*
 * Glane — Copyright 2026 Baptiste Faisy. Licence Apache-2.0.
 *
 * test_util.h — micro-harnais de test (GLA-01, §7 du cahier).
 *
 * Convention (identique pour tous les tests de core/) :
 *   - un binaire = un fichier .c = une translation unit ;
 *   - `static` g_checks / g_fails propres à chaque binaire (header inclus
 *     une seule fois par fichier de test) ;
 *   - le binaire imprime "OK <n> checks" et retourne 0 si tout est vert ;
 *   - retourne 1 et imprime "FAIL <n>/<m>" sur la première ligne d'échec.
 *
 * Le testeur ne dépend d'aucune bibliothèque externe (stdio/string seulement).
 */
#ifndef GLANE_TEST_UTIL_H
#define GLANE_TEST_UTIL_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned g_checks = 0;
static unsigned g_fails  = 0;

/* Vrai si la condition est vraie. Compte, ne sort pas du test. */
#define CHECK(cond) do {                                                 \
    if (cond) { g_checks++; }                                            \
    else {                                                               \
        g_fails++;                                                       \
        fprintf(stderr, "FAIL %s:%d: %s\n",                              \
                __FILE__, __LINE__, #cond);                              \
    }                                                                    \
} while (0)

/* Comparaison entière ; imprime les valeurs en cas d'échec. */
#define CHECK_EQ_I(a, b) do {                                            \
    long long _va = (long long)(a);                                      \
    long long _vb = (long long)(b);                                      \
    if (_va == _vb) { g_checks++; }                                      \
    else {                                                               \
        g_fails++;                                                       \
        fprintf(stderr, "FAIL %s:%d: %s != %s (got %lld, want %lld)\n",  \
                __FILE__, __LINE__, #a, #b, _va, _vb);                   \
    }                                                                    \
} while (0)

/* Comparaison de chaînes (strcmp == 0). */
#define CHECK_EQ_S(a, b) do {                                            \
    if ((a) && (b) && strcmp((a), (b)) == 0) { g_checks++; }             \
    else {                                                               \
        g_fails++;                                                       \
        fprintf(stderr, "FAIL %s:%d: \"%s\" != \"%s\"\n",                \
                __FILE__, __LINE__,                                      \
                (a) ? (a) : "(null)", (b) ? (b) : "(null)");             \
    }                                                                    \
} while (0)

/* Termine le test : verdict + code retour. À appeler une fois en fin de
 * main(). Retourne depuis main, donc ne pas mettre après d'autres CHECK. */
#define TEST_DONE() do {                                                 \
    if (g_fails) {                                                       \
        fprintf(stderr, "FAIL %u/%u\n", g_fails, g_checks + g_fails);    \
        return 1;                                                        \
    }                                                                    \
    printf("OK %u checks\n", g_checks);                                  \
    return 0;                                                            \
} while (0)

#endif /* GLANE_TEST_UTIL_H */