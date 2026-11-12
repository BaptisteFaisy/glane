/*
 * Glane — Copyright 2026 Baptiste Faisy. Licence Apache-2.0.
 *
 * glane_selftest.h — selftest sans état sur vecteurs de référence (idée A21).
 *
 * Un selftest qui rejoue des vecteurs de référence versionnés pour chaque
 * sous-système du socle et retourne un bitset des échecs. Sert de base à la
 * commande publiée `/dev/sensors/<name>/command/selftest` et au diagnostic
 * terrain : 0 = tout est vert, sans aucun état externe ni allocation.
 */
#ifndef GLANE_SELFTEST_H
#define GLANE_SELFTEST_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Version du jeu de vecteurs de référence (incrémentée à chaque changement). */
#define GLANE_SELFTEST_VECTORS_VERSION 1

/* Bits du bitset d'échec (un par sous-système). */
enum {
    GLANE_SELFTEST_CALIB    = 1u << 0,
    GLANE_SELFTEST_SAMPLE   = 1u << 1,
    GLANE_SELFTEST_VIRTUEL  = 1u << 2,
    GLANE_SELFTEST_DEADLINE = 1u << 3,
    GLANE_SELFTEST_SATURATE = 1u << 4,
};

/* Rejoue tous les vecteurs de référence. Retourne un bitset des sous-systèmes
 * en échec ; 0 signifie que tout est vert. Sans état, sans allocation.     */
uint32_t glane_selftest(void);

#ifdef __cplusplus
}
#endif

#endif /* GLANE_SELFTEST_H */