/*
 * Glane — Copyright 2026 Baptiste Faisy. Licence Apache-2.0.
 *
 * test_virtuel.c — vérifie le générateur synthétique déterministe (GLA-03/04).
 */
#include "glane/glane_virtuel.h"
#include "test_util.h"

#include <limits.h>

static glane_virtuel_cfg cfg_base(int32_t wf, int32_t amp, int32_t period)
{
    glane_virtuel_cfg c;
    c.sensor_id = 1;
    c.unit = GLANE_UNIT_C;
    c.wf = wf;
    c.base_milli = 20000;       /* 20.0 unités */
    c.amp_milli = amp;
    c.period_steps = period;
    c.drift_per_step = 0;
    c.seed = 12345;
    c.sat_min_milli = -40000;   /* -40.0 */
    c.sat_max_milli = 125000;   /* 125.0 */
    return c;
}

static void test_const(void)
{
    glane_virtuel v;
    glane_virtuel_cfg c = cfg_base(GLANE_WF_CONST, 0, 1);
    glane_virtuel_init(&v, &c);
    glane_sample s;
    for (int i = 0; i < 5; i++) {
        CHECK_EQ_I(glane_virtuel_next(&v, (int64_t)i * 1000, &s), 0);
        CHECK_EQ_I(s.value_milli, 20000);
        CHECK_EQ_I(s.quality, GLANE_Q_OK);
        CHECK_EQ_I(s.seq, (uint32_t)(i + 1));
    }
}

static void test_sine_range(void)
{
    glane_virtuel v;
    glane_virtuel_cfg c = cfg_base(GLANE_WF_SINE, 10000, 64);
    glane_virtuel_init(&v, &c);
    glane_sample s;
    int saw_high = 0, saw_low = 0;
    for (int i = 0; i < 64; i++) {
        glane_virtuel_next(&v, i, &s);
        /* base 20000 +/- amp 10000 -> [10000, 30000] */
        CHECK(s.value_milli >= 10000 - 1 && s.value_milli <= 30000 + 1);
        if (s.value_milli > 29000) saw_high = 1;
        if (s.value_milli < 11000) saw_low = 1;
    }
    CHECK(saw_high);  /* la sinusoïde monte bien */
    CHECK(saw_low);   /* et descend */
}

static void test_ramp_and_triangle(void)
{
    glane_virtuel v;
    glane_virtuel_cfg c = cfg_base(GLANE_WF_RAMP, 5000, 10);
    glane_virtuel_init(&v, &c);
    glane_sample s;
    /* dent de scie : phase 0 -> -amp, varie sur la période */
    glane_virtuel_next(&v, 0, &s);
    CHECK_EQ_I(s.value_milli, 20000 - 5000);  /* base + (-amp) */
    c.wf = GLANE_WF_TRIANGLE;
    glane_virtuel_init(&v, &c);
    glane_virtuel_next(&v, 0, &s);
    CHECK_EQ_I(s.value_milli, 20000 - 5000);  /* triangle démarre à -amp */
}

static void test_noise_bounded_and_seeded(void)
{
    glane_virtuel v;
    glane_virtuel_cfg c = cfg_base(GLANE_WF_NOISE, 8000, 1);
    glane_virtuel_init(&v, &c);
    glane_sample s;
    for (int i = 0; i < 200; i++) {
        glane_virtuel_next(&v, i, &s);
        CHECK(s.value_milli >= 20000 - 8000 && s.value_milli <= 20000 + 8000);
    }
    /* Même graine -> même suite (déterminisme). */
    glane_virtuel v2;
    glane_virtuel_init(&v2, &c);
    glane_sample s2;
    int same = 1;
    for (int i = 0; i < 200; i++) {
        glane_virtuel_next(&v, 0, &s);   /* v a déjà avancé ; on ré-init */
        (void)s;
        break;
    }
    /* compare deux générateurs frais */
    glane_virtuel_init(&v, &c);
    glane_virtuel_init(&v2, &c);
    for (int i = 0; i < 50; i++) {
        glane_virtuel_next(&v, i, &s);
        glane_virtuel_next(&v2, i, &s2);
        if (s.value_milli != s2.value_milli) same = 0;
    }
    CHECK(same);
    /* Graine différente -> suite différente (au moins une valeur). */
    c.seed = 999;
    glane_virtuel v3;
    glane_virtuel_init(&v3, &c);
    c.seed = 12345;
    glane_virtuel_init(&v, &c);
    int differ = 0;
    for (int i = 0; i < 50; i++) {
        glane_virtuel_next(&v, i, &s);
        glane_virtuel_next(&v3, i, &s2);
        if (s.value_milli != s2.value_milli) differ = 1;
    }
    CHECK(differ);
}

static void test_saturation(void)
{
    glane_virtuel v;
    glane_virtuel_cfg c = cfg_base(GLANE_WF_CONST, 0, 1);
    c.base_milli = 200000;      /* au-dessus de sat_max 125000 */
    glane_virtuel_init(&v, &c);
    glane_sample s;
    glane_virtuel_next(&v, 0, &s);
    CHECK_EQ_I(s.value_milli, 125000);
    CHECK_EQ_I(s.quality, GLANE_Q_SATURATED);
    c.base_milli = -99999;      /* sous sat_min -40000 */
    glane_virtuel_init(&v, &c);
    glane_virtuel_next(&v, 0, &s);
    CHECK_EQ_I(s.value_milli, -40000);
    CHECK_EQ_I(s.quality, GLANE_Q_SATURATED);
    /* Bornes exactes : atteignables sans saturation (sinon la borne haute
     * serait exclusive et la basse déclenchée à tort). */
    c.base_milli = 125000;      /* == sat_max */
    glane_virtuel_init(&v, &c);
    glane_virtuel_next(&v, 0, &s);
    CHECK_EQ_I(s.value_milli, 125000);
    CHECK_EQ_I(s.quality, GLANE_Q_OK);
    c.base_milli = -40000;      /* == sat_min */
    glane_virtuel_init(&v, &c);
    glane_virtuel_next(&v, 0, &s);
    CHECK_EQ_I(s.value_milli, -40000);
    CHECK_EQ_I(s.quality, GLANE_Q_OK);
}

static void test_drift_and_first_flag(void)
{
    glane_virtuel v;
    glane_virtuel_cfg c = cfg_base(GLANE_WF_CONST, 0, 1);
    c.drift_per_step = 100;     /* +100 milli par pas */
    glane_virtuel_init(&v, &c);
    glane_sample s;
    glane_virtuel_next(&v, 0, &s);
    CHECK_EQ_I(s.value_milli, 20000);
    CHECK(s.flags & GLANE_F_FIRST);          /* première mesure */
    CHECK(s.flags & GLANE_F_SIMULATED);
    glane_virtuel_next(&v, 1000, &s);
    CHECK_EQ_I(s.value_milli, 20100);
    CHECK(!(s.flags & GLANE_F_FIRST));       /* plus la première */
    glane_virtuel_next(&v, 2000, &s);
    CHECK_EQ_I(s.value_milli, 20200);
}

static void test_determinism_full(void)
{
    /* Deux générateurs identiques -> journaux identiques (GLA-04). */
    glane_virtuel_cfg c = cfg_base(GLANE_WF_SINE, 12000, 32);
    c.drift_per_step = 7;
    glane_virtuel a, b;
    glane_virtuel_init(&a, &c);
    glane_virtuel_init(&b, &c);
    glane_sample sa, sb;
    int identical = 1;
    for (int i = 0; i < 100; i++) {
        glane_virtuel_next(&a, (int64_t)i * 1000000, &sa);
        glane_virtuel_next(&b, (int64_t)i * 1000000, &sb);
        if (sa.value_milli != sb.value_milli || sa.seq != sb.seq ||
            sa.ts_ns != sb.ts_ns || sa.quality != sb.quality ||
            sa.flags != sb.flags) {
            identical = 0;
        }
    }
    CHECK(identical);
}

static void test_null_safety(void)
{
    glane_virtuel v;
    glane_virtuel_cfg c = cfg_base(GLANE_WF_CONST, 0, 1);
    glane_virtuel_init(NULL, &c);
    glane_virtuel_init(&v, NULL);
    glane_sample s;
    CHECK_EQ_I(glane_virtuel_next(NULL, 0, &s), -1);
    glane_virtuel_init(&v, &c);
    CHECK_EQ_I(glane_virtuel_next(&v, 0, NULL), -1);
    CHECK(1);
}

int main(void)
{
    test_const();
    test_sine_range();
    test_ramp_and_triangle();
    test_noise_bounded_and_seeded();
    test_saturation();
    test_drift_and_first_flag();
    test_determinism_full();
    test_null_safety();
    TEST_DONE();
}