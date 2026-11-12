/*
 * Glane — Copyright 2026 Baptiste Faisy. Licence Apache-2.0.
 *
 * selftest.c — selftest sans état sur vecteurs de référence (idée A21).
 * Chaque sous-système rejoue des vecteurs connus ; tout écart lève son bit.
 */
#include "glane/glane_selftest.h"

#include "glane/glane_calib.h"
#include "glane/glane_deadline.h"
#include "glane/glane_sample.h"
#include "glane/glane_saturate.h"
#include "glane/glane_virtuel.h"

#include <limits.h>

static uint32_t st_calib(void)
{
    glane_calib c = { { 0, 1000000, 0, 0 }, 1, GLANE_CALIB_POLY };
    if (glane_calib_apply(&c, 12345) != 12345) {
        return GLANE_SELFTEST_CALIB;  /* gain 1.0 = identité */
    }
    glane_calib sq = { { 0, 0, 1000000, 0 }, 2, GLANE_CALIB_POLY };
    if (glane_calib_apply(&sq, 10) != 100) {
        return GLANE_SELFTEST_CALIB;  /* x^2 : 10 -> 100 */
    }
    glane_calib none = { { 0, 0, 0, 0 }, 0, GLANE_CALIB_NONE };
    if (glane_calib_apply(&none, -7) != -7) {
        return GLANE_SELFTEST_CALIB;
    }
    return 0;
}

static uint32_t st_sample(void)
{
    glane_sample s;
    glane_sample_init(&s, 3, GLANE_UNIT_C);
    if (s.seq != 0 || s.quality != GLANE_Q_OK || s.sensor_id != 3) {
        return GLANE_SELFTEST_SAMPLE;
    }
    if (glane_sample_emit(&s, 1000, 500) != 0 || s.seq != 1) {
        return GLANE_SELFTEST_SAMPLE;
    }
    if (glane_sample_emit(&s, 1000, 400) != -1) {
        return GLANE_SELFTEST_SAMPLE;  /* retour en arrière refusé */
    }
    if (sizeof(glane_sample) != GLANE_SAMPLE_SIZE) {
        return GLANE_SELFTEST_SAMPLE;
    }
    return 0;
}

static uint32_t st_virtuel(void)
{
    glane_virtuel_cfg cfg = {
        1, GLANE_UNIT_C, GLANE_WF_CONST, 20000, 0, 1, 0, 7, -40000, 125000
    };
    glane_virtuel v;
    glane_virtuel_init(&v, &cfg);
    glane_sample s;
    if (glane_virtuel_next(&v, 0, &s) != 0 || s.value_milli != 20000) {
        return GLANE_SELFTEST_VIRTUEL;
    }
    if (!(s.flags & GLANE_F_SIMULATED) || !(s.flags & GLANE_F_FIRST)) {
        return GLANE_SELFTEST_VIRTUEL;
    }
    /* Déterminisme : deux générateurs identiques -> même valeur. */
    glane_virtuel v2;
    glane_virtuel_init(&v2, &cfg);
    glane_sample s2;
    glane_virtuel_next(&v2, 0, &s2);
    if (s2.value_milli != s.value_milli) {
        return GLANE_SELFTEST_VIRTUEL;
    }
    return 0;
}

static uint32_t st_deadline(void)
{
    glane_deadline d;
    glane_deadline_init(&d, 100, 500, 2);
    if (glane_deadline_quality(&d, 100LL * 1000000LL) != GLANE_Q_OK) {
        return GLANE_SELFTEST_DEADLINE;
    }
    if (glane_deadline_quality(&d, 600LL * 1000000LL) != GLANE_Q_STALE) {
        return GLANE_SELFTEST_DEADLINE;
    }
    if (glane_deadline_missed(&d) != GLANE_Q_STALE) {
        return GLANE_SELFTEST_DEADLINE;
    }
    if (glane_deadline_missed(&d) != GLANE_Q_FAULT) {
        return GLANE_SELFTEST_DEADLINE;
    }
    return 0;
}

static uint32_t st_saturate(void)
{
    uint8_t sat = 0;
    if (glane_sat_add_i32(INT32_MAX, 1, &sat) != INT32_MAX || sat != 1) {
        return GLANE_SELFTEST_SATURATE;
    }
    if (glane_sat_add_i32(2, 3, &sat) != 5 || sat != 0) {
        return GLANE_SELFTEST_SATURATE;
    }
    return 0;
}

uint32_t glane_selftest(void)
{
    uint32_t fail = 0;
    fail |= st_calib();
    fail |= st_sample();
    fail |= st_virtuel();
    fail |= st_deadline();
    fail |= st_saturate();
    return fail;
}