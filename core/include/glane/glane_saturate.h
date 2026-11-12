/*
 * Glane — Copyright 2026 Baptiste Faisy. Licence Apache-2.0.
 *
 * glane_saturate.h — arithmétique saturante contractuelle (idée A1).
 *
 * Un dépassement ne produit JAMAIS de wraparound silencieux : il sature à la
 * borne et lève un drapeau, que l'appelant traduit en `quality = SATURATED`.
 * C'est la garantie « aucune valeur fausse n'est publiée sans être signalée ».
 */
#ifndef GLANE_SATURATE_H
#define GLANE_SATURATE_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* a + b avec saturation int32. *saturated (si non NULL) vaut 1 en cas de
 * dépassement, 0 sinon. Retourne la somme saturée.                        */
int32_t glane_sat_add_i32(int32_t a, int32_t b, uint8_t *saturated);

/* a * b avec saturation int32. */
int32_t glane_sat_mul_i32(int32_t a, int32_t b, uint8_t *saturated);

/* Ramène v (int64) dans [lo, hi] avec saturation ; *saturated signale le
 * clamp. Retourne la valeur dans int32.                                   */
int32_t glane_sat_clamp_i64(int64_t v, int32_t lo, int32_t hi,
                            uint8_t *saturated);

#ifdef __cplusplus
}
#endif

#endif /* GLANE_SATURATE_H */