/*
 * Glane — Copyright 2026 Baptiste Faisy. Licence Apache-2.0.
 *
 * calib_dump.c — utilitaire hôte pour le test différentiel C↔Python (GLA-05).
 * Lit sur stdin des lignes « deg c0 c1 c2 c3 x » (coefficients échelle 1e6,
 * x en milli) et imprime le résultat de glane_calib_apply, une valeur par
 * ligne. Le jumeau Python (host/glane_twin.py) compare ses propres résultats.
 * Ce n'est pas du core/ : les E/S sont autorisées ici.
 */
#include "glane/glane_calib.h"

#include <stdio.h>

int main(void)
{
    long long deg, c0, c1, c2, c3, x;
    while (scanf("%lld %lld %lld %lld %lld %lld",
                 &deg, &c0, &c1, &c2, &c3, &x) == 6) {
        glane_calib cal;
        cal.kind = GLANE_CALIB_POLY;
        cal.deg = (int32_t)deg;
        cal.coef[0] = (int64_t)c0;
        cal.coef[1] = (int64_t)c1;
        cal.coef[2] = (int64_t)c2;
        cal.coef[3] = (int64_t)c3;
        int32_t r = glane_calib_apply(&cal, (int32_t)x);
        printf("%d\n", r);
    }
    return 0;
}