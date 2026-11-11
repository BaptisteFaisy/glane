/*
 * Glane — Copyright 2026 Baptiste Faisy. Licence Apache-2.0.
 *
 * calib.c — calibration en arithmétique entière (GLA-05).
 *
 * Polynôme de degré <= 3, coefficients entiers échelle 10^6, accumulateur
 * 128 bits (extension __int128, supportée par gcc/clang sur x86-64 et
 * aarch64 — donc par le toolchain QNX). Aucun terme ne déborde dans les
 * bornes validées par desc.c (CONTRACT.md §2.1). Le jumeau Python (L3)
 * refait la même arithmétique entière avec des int de précision arbitraire.
 */
#include "glane/glane_calib.h"

#include <limits.h>
#include <stddef.h>

/* __int128 est une extension gcc/clang, silencieuse sous -std=c11 -Wall
 * -Wextra -Werror (sans -pedantic). Documenté dans CONTRACT.md §2.        */
typedef __int128 int128_t;

/* Arrondi au plus proche, demi à l'infini (half away from zero), puis
 * clamp dans int32. num/den sont signés ; den > 0.                        */
static int32_t round_div_clamp(int128_t num, int64_t den)
{
    int128_t half = (int128_t)(den / 2);
    if (num >= 0) {
        num += half;
    } else {
        num -= half;
    }
    int128_t q = num / (int128_t)den;
    if (q > (int128_t)INT32_MAX) {
        return INT32_MAX;
    }
    if (q < (int128_t)INT32_MIN) {
        return INT32_MIN;
    }
    return (int32_t)q;
}

int32_t glane_calib_apply(const glane_calib *c, int32_t x_milli)
{
    if (c == NULL || c->kind == GLANE_CALIB_NONE) {
        return x_milli;  /* identité */
    }
    int deg = c->deg;
    if (deg < 0) {
        deg = 0;
    }
    if (deg > GLANE_CALIB_MAX_DEG) {
        deg = GLANE_CALIB_MAX_DEG;
    }
    int128_t x = (int128_t)(int32_t)x_milli;
    int128_t acc = 0;
    /* Horner : acc = coef[deg]; puis acc = acc*x + coef[i] pour i décroissant.
     * À la fin acc = somme coef[i] * x^i, tout en échelle GLANE_CALIB_SCALE. */
    acc = (int128_t)c->coef[deg];
    for (int i = deg - 1; i >= 0; i--) {
        acc = acc * x + (int128_t)c->coef[i];
    }
    /* p(x) = acc / SCALE, arrondi au plus proche (demi à l'infini).       */
    return round_div_clamp(acc, (int64_t)GLANE_CALIB_SCALE);
}

/* Puissance de 10 exacte en int64 (10^0..10^18 tiennent). Retourne -1 si
 * l'exposant sort de la plage supportée.                                  */
static int64_t pow10(int exp)
{
    static const int64_t table[19] = {
        1LL, 10LL, 100LL, 1000LL, 10000LL, 100000LL, 1000000LL,
        10000000LL, 100000000LL, 1000000000LL, 10000000000LL,
        100000000000LL, 1000000000000LL, 10000000000000LL,
        100000000000000LL, 1000000000000000LL, 10000000000000000LL,
        100000000000000000LL, 1000000000000000000LL,
    };
    if (exp < 0 || exp > 18) {
        return -1;
    }
    return table[exp];
}

/* Parse un décimal signé optionnellement avec exposant en entier échelle
 * 10^6. Étapes : signe, chiffres (significande), point décimal, exposant,
 * puis mise à l'échelle 10^6 avec arrondi au plus proche. Refuse tout
 * caractère parasite ou tout débordement.                                */
/* Parse signe + chiffres + point décimal. Avance *pp. Retourne 0 si OK. */
static int parse_significand(const char **pp, int64_t *sig, int *fd, int *neg)
{
    const char *p = *pp;
    *neg = 0;
    if (*p == '+') {
        p++;
    } else if (*p == '-') {
        *neg = 1;
        p++;
    }
    *sig = 0;
    *fd = 0;
    int digits = 0;
    int sawdot = 0;
    for (; *p != '\0'; p++) {
        if (*p >= '0' && *p <= '9') {
            *sig = *sig * 10 + (int64_t)(*p - '0');
            digits++;
            if (sawdot) {
                (*fd)++;
            }
            if (digits > 18) {
                return 2;  /* significande trop grand pour int64 */
            }
        } else if (*p == '.' && !sawdot) {
            sawdot = 1;
        } else {
            break;
        }
    }
    if (digits == 0) {
        return 3;
    }
    *pp = p;
    return 0;
}

/* Parse l'exposant optionnel (e/E). Avance *pp. Retourne 0 si OK. */
static int parse_exponent(const char **pp, int *exp)
{
    const char *p = *pp;
    *exp = 0;
    if (*p != 'e' && *p != 'E') {
        return 0;
    }
    p++;
    int esign = 1;
    if (*p == '+') {
        p++;
    } else if (*p == '-') {
        esign = -1;
        p++;
    }
    int sawe = 0;
    for (; *p >= '0' && *p <= '9'; p++) {
        *exp = *exp * 10 + (*p - '0');
        sawe = 1;
        if (*exp > 30) {
            return 4;
        }
    }
    if (!sawe) {
        return 4;
    }
    *exp *= esign;
    *pp = p;
    return 0;
}

/* Met le significande à l'échelle 10^6 compte tenu des décimales (fd) et de
 * l'exposant (exp). Arrondi au plus proche, demi à l'infini.              */
static int scale_to_micro(int64_t sig, int fd, int exp, int neg, int64_t *out)
{
    int total = exp - fd + 6;
    int64_t scaled;
    if (total >= 0) {
        int64_t m = pow10(total);
        if (m < 0) {
            return 4;
        }
        if (sig != 0 && m > INT64_MAX / sig) {
            return 2;  /* débordement */
        }
        scaled = sig * m;
    } else {
        int64_t d = pow10(-total);
        if (d < 0) {
            return 4;
        }
        scaled = (sig + d / 2) / d;
    }
    *out = neg ? -scaled : scaled;
    return 0;
}

int glane_calib_parse_coef(const char *s, int64_t *out)
{
    if (s == NULL || out == NULL) {
        return 1;
    }
    const char *p = s;
    int64_t sig;
    int fd, neg, exp, rc;
    if ((rc = parse_significand(&p, &sig, &fd, &neg)) != 0) {
        return rc;
    }
    if ((rc = parse_exponent(&p, &exp)) != 0) {
        return rc;
    }
    if (*p != '\0') {
        return 5;  /* caractère parasite */
    }
    return scale_to_micro(sig, fd, exp, neg, out);
}

const char *glane_calib_err(int code)
{
    switch (code) {
    case 0:  return "ok";
    case 1:  return "pointeur NULL";
    case 2:  return "nombre trop grand (débordement int64)";
    case 3:  return "aucun chiffre";
    case 4:  return "exposant invalide";
    case 5:  return "caractère parasite après le nombre";
    default: return "erreur inconnue";
    }
}