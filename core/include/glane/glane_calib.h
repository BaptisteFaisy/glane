/*
 * Glane — Copyright 2026 Baptiste Faisy. Licence Apache-2.0.
 *
 * glane_calib.h — calibration en arithmétique entière (GLA-05, §5.1).
 *
 * La calibration est un polynôme de degré <= 3 appliqué à une entrée brute
 * exprimée en milli-unités. Les coefficients sont stockés en entier, échelle
 * GLANE_CALIB_SCALE = 10^6 (donc 1.0 -> 1 000 000). L'accumulateur interne est
 * 128 bits : le produit d'un terme de degré 3 ne peut pas déborder, et le
 * résultat est bit à bit identique au jumeau Python (qui refait la même
 * arithmétique entière avec des entiers de précision arbitraire).
 */
#ifndef GLANE_CALIB_H
#define GLANE_CALIB_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define GLANE_CALIB_SCALE 1000000LL  /* échelle fixe des coefficients       */
#define GLANE_CALIB_MAX_DEG 3        /* degré maximum du polynôme           */

/* Type de calibration. OFFSET = polynôme de degré 0 (une constante).      */
enum {
    GLANE_CALIB_NONE   = 0,  /* identité : valeur = brute                   */
    GLANE_CALIB_OFFSET = 1,  /* valeur = brute + coef[0]                    */
    GLANE_CALIB_POLY   = 2,  /* polynôme de degré deg (0..3)               */
};

typedef struct {
    int64_t coef[4];  /* coefficients, échelle GLANE_CALIB_SCALE           */
    int32_t deg;      /* degré effectif (0..GLANE_CALIB_MAX_DEG)           */
    int32_t kind;     /* GLANE_CALIB_NONE / _OFFSET / _POLY                */
} glane_calib;

/* Applique la calibration. x_milli = entrée brute en milli-unités.
 * Retourne la sortie en milli-unités (arrondie au plus proche, demi à
 * l'infini). NONE = identité. Le comportement est borné et déterminé.     */
int32_t glane_calib_apply(const glane_calib *c, int32_t x_milli);

/* Parse une valeur décimale signée ("1.0", "-0.5", "125", ...) en entier
 * échelle GLANE_CALIB_SCALE. Accepte un signe, des décimales quelconques,
 * un exposant optionnel (e/E). Arrondit au plus proche. Retourne 0 si OK,
 * sinon un code non nul (voir glane_calib_err).                           */
int glane_calib_parse_coef(const char *s, int64_t *out_milli_scaled);

/* Message court pour un code d'erreur de glane_calib_parse_coef (statique). */
const char *glane_calib_err(int code);

#ifdef __cplusplus
}
#endif

#endif /* GLANE_CALIB_H */