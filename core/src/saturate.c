/*
 * Glane — Copyright 2026 Baptiste Faisy. Licence Apache-2.0.
 *
 * saturate.c — arithmétique saturante (idée A1). Voir glane_saturate.h.
 */
#include "glane/glane_saturate.h"

#include <limits.h>
#include <stddef.h>

/* Ramène un int64 dans int32 en signalant la saturation. */
static int32_t clamp_i32(int64_t v, uint8_t *saturated)
{
    int sat = 0;
    if (v > (int64_t)INT32_MAX) {
        v = INT32_MAX;
        sat = 1;
    } else if (v < (int64_t)INT32_MIN) {
        v = INT32_MIN;
        sat = 1;
    }
    if (saturated != NULL) {
        *saturated = (uint8_t)sat;
    }
    return (int32_t)v;
}

int32_t glane_sat_add_i32(int32_t a, int32_t b, uint8_t *saturated)
{
    return clamp_i32((int64_t)a + (int64_t)b, saturated);
}

int32_t glane_sat_mul_i32(int32_t a, int32_t b, uint8_t *saturated)
{
    return clamp_i32((int64_t)a * (int64_t)b, saturated);
}

int32_t glane_sat_clamp_i64(int64_t v, int32_t lo, int32_t hi,
                            uint8_t *saturated)
{
    int sat = 0;
    if (lo <= hi) {
        if (v < (int64_t)lo) {
            v = lo;
            sat = 1;
        } else if (v > (int64_t)hi) {
            v = hi;
            sat = 1;
        }
    }
    /* Saturation finale dans int32 (lo/hi sont déjà int32, mais v peut
     * déborder int32 avant clamp si lo>hi — garde-fou). */
    if (v > (int64_t)INT32_MAX) {
        v = INT32_MAX;
        sat = 1;
    } else if (v < (int64_t)INT32_MIN) {
        v = INT32_MIN;
        sat = 1;
    }
    if (saturated != NULL) {
        *saturated = (uint8_t)sat;
    }
    return (int32_t)v;
}