/*
 * Glane — Copyright 2026 Baptiste Faisy. Licence Apache-2.0.
 *
 * test_saturate.c — vérifie l'arithmétique saturante (idée A1).
 */
#include "glane/glane_saturate.h"
#include "test_util.h"

#include <limits.h>

static void test_add(void)
{
    uint8_t sat = 9;
    CHECK_EQ_I(glane_sat_add_i32(10, 20, &sat), 30);
    CHECK_EQ_I(sat, 0);
    CHECK_EQ_I(glane_sat_add_i32(-10, -20, &sat), -30);
    CHECK_EQ_I(sat, 0);
    /* Dépassement haut -> INT32_MAX + drapeau. */
    CHECK_EQ_I(glane_sat_add_i32(INT32_MAX, 1, &sat), INT32_MAX);
    CHECK_EQ_I(sat, 1);
    CHECK_EQ_I(glane_sat_add_i32(INT32_MAX, INT32_MAX, &sat), INT32_MAX);
    CHECK_EQ_I(sat, 1);
    /* Dépassement bas -> INT32_MIN + drapeau. */
    CHECK_EQ_I(glane_sat_add_i32(INT32_MIN, -1, &sat), INT32_MIN);
    CHECK_EQ_I(sat, 1);
    /* Limite exacte : pas de saturation. */
    CHECK_EQ_I(glane_sat_add_i32(INT32_MAX - 1, 1, &sat), INT32_MAX);
    CHECK_EQ_I(sat, 0);
    /* NULL saturated autorisé. */
    CHECK_EQ_I(glane_sat_add_i32(INT32_MAX, 5, NULL), INT32_MAX);
}

static void test_mul(void)
{
    uint8_t sat = 9;
    CHECK_EQ_I(glane_sat_mul_i32(6, 7, &sat), 42);
    CHECK_EQ_I(sat, 0);
    CHECK_EQ_I(glane_sat_mul_i32(-6, 7, &sat), -42);
    CHECK_EQ_I(sat, 0);
    CHECK_EQ_I(glane_sat_mul_i32(100000, 100000, &sat), INT32_MAX);
    CHECK_EQ_I(sat, 1);
    CHECK_EQ_I(glane_sat_mul_i32(-100000, 100000, &sat), INT32_MIN);
    CHECK_EQ_I(sat, 1);
    CHECK_EQ_I(glane_sat_mul_i32(0, INT32_MAX, &sat), 0);
    CHECK_EQ_I(sat, 0);
}

static void test_clamp(void)
{
    uint8_t sat = 9;
    CHECK_EQ_I(glane_sat_clamp_i64(50, 0, 100, &sat), 50);
    CHECK_EQ_I(sat, 0);
    CHECK_EQ_I(glane_sat_clamp_i64(150, 0, 100, &sat), 100);
    CHECK_EQ_I(sat, 1);
    CHECK_EQ_I(glane_sat_clamp_i64(-50, 0, 100, &sat), 0);
    CHECK_EQ_I(sat, 1);
    /* Bornes égales : tout est clampé à cette valeur. */
    CHECK_EQ_I(glane_sat_clamp_i64(999, 7, 7, &sat), 7);
    CHECK_EQ_I(sat, 1);
    /* lo > hi : saturation désactivée, mais clamp int32 toujours actif. */
    CHECK_EQ_I(glane_sat_clamp_i64(5000, 100, 0, &sat), 5000);
    CHECK_EQ_I(sat, 0);
    /* Grand int64 -> clamp int32. */
    CHECK_EQ_I(glane_sat_clamp_i64((int64_t)INT64_MAX, INT32_MIN, INT32_MAX,
                                   &sat), INT32_MAX);
    CHECK_EQ_I(sat, 1);
}

int main(void)
{
    test_add();
    test_mul();
    test_clamp();
    TEST_DONE();
}