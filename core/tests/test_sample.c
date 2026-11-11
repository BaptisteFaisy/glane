/*
 * Glane — Copyright 2026 Baptiste Faisy. Licence Apache-2.0.
 *
 * test_sample.c — vérifie l'ABI de la mesure et les helpers (GLA-03).
 */
#include "glane/glane_sample.h"
#include "test_util.h"

static void test_size_and_layout(void)
{
    CHECK_EQ_I(sizeof(glane_sample), GLANE_SAMPLE_SIZE);
    /* Aucun pointeur dans le POD : la taille est exactement 24. */
    glane_sample s;
    glane_sample_init(&s, 7, GLANE_UNIT_C);
    CHECK_EQ_I(s.sensor_id, 7u);
    CHECK_EQ_I(s.unit, GLANE_UNIT_C);
    CHECK_EQ_I(s.seq, 0u);
    CHECK_EQ_I(s.ts_ns, 0);
    CHECK_EQ_I(s.value_milli, 0);
    CHECK_EQ_I(s.quality, GLANE_Q_OK);
    CHECK_EQ_I(s.flags, 0);
}

static void test_names(void)
{
    CHECK_EQ_S(glane_unit_name(GLANE_UNIT_C), "C");
    CHECK_EQ_S(glane_unit_name(GLANE_UNIT_K), "K");
    CHECK_EQ_S(glane_unit_name(GLANE_UNIT_V), "V");
    CHECK_EQ_S(glane_unit_name(GLANE_UNIT_MA), "mA");
    CHECK_EQ_S(glane_unit_name(GLANE_UNIT_DEG_S), "deg/s");
    CHECK_EQ_S(glane_unit_name(GLANE_UNIT_G), "g");
    CHECK_EQ_S(glane_unit_name(GLANE_UNIT_RPM), "rpm");
    CHECK_EQ_S(glane_unit_name(GLANE_UNIT_PCT), "%");
    CHECK_EQ_S(glane_unit_name(GLANE_UNIT_COUNT), "count");
    CHECK_EQ_S(glane_unit_name(9999), "?");
    CHECK_EQ_S(glane_quality_name(GLANE_Q_OK), "ok");
    CHECK_EQ_S(glane_quality_name(GLANE_Q_STALE), "stale");
    CHECK_EQ_S(glane_quality_name(GLANE_Q_SATURATED), "saturated");
    CHECK_EQ_S(glane_quality_name(GLANE_Q_FAULT), "fault");
    CHECK_EQ_S(glane_quality_name(GLANE_Q_UNCERTAIN), "uncertain");
    CHECK_EQ_S(glane_quality_name(200), "?");
}

static void test_emit_seq_monotone(void)
{
    glane_sample s;
    glane_sample_init(&s, 1, GLANE_UNIT_C);
    /* 50 émissions : seq doit avancer de 1 à chaque fois, ts monotone. */
    int64_t ts = 1000;
    for (int i = 0; i < 50; i++) {
        int rc = glane_sample_emit(&s, (int32_t)(i * 10), ts);
        CHECK_EQ_I(rc, 0);
        CHECK_EQ_I(s.seq, (uint32_t)(i + 1));
        CHECK_EQ_I(s.ts_ns, ts);
        CHECK_EQ_I(s.value_milli, i * 10);
        CHECK_EQ_I(s.quality, GLANE_Q_OK);
        ts += 100;
    }
    /* Retour en arrière de l'horodatage : refusé, seq inchangé. */
    uint32_t seq_before = s.seq;
    int rc = glane_sample_emit(&s, 999, ts - 1000);
    CHECK_EQ_I(rc, -1);
    CHECK_EQ_I(s.seq, seq_before);
    /* ts identique (égalité) : accepté, seq avance quand même. */
    rc = glane_sample_emit(&s, 1, ts);
    CHECK_EQ_I(rc, 0);
    CHECK_EQ_I(s.seq, seq_before + 1);
}

static void test_flags(void)
{
    glane_sample s;
    glane_sample_init(&s, 2, GLANE_UNIT_V);
    glane_sample_flag(&s, GLANE_F_FIRST, 1);
    CHECK_EQ_I(s.flags & GLANE_F_FIRST, GLANE_F_FIRST);
    glane_sample_flag(&s, GLANE_F_CALIBRATED, 1);
    CHECK_EQ_I(s.flags & GLANE_F_CALIBRATED, GLANE_F_CALIBRATED);
    CHECK_EQ_I(s.flags & GLANE_F_FIRST, GLANE_F_FIRST);  /* cumulable */
    glane_sample_flag(&s, GLANE_F_FIRST, 0);
    CHECK_EQ_I(s.flags & GLANE_F_FIRST, 0);
    CHECK_EQ_I(s.flags & GLANE_F_CALIBRATED, GLANE_F_CALIBRATED);  /* préservé */
    /* Tous les bits ensemble. */
    s.flags = 0;
    glane_sample_flag(&s, GLANE_F_FIRST, 1);
    glane_sample_flag(&s, GLANE_F_CALIBRATED, 1);
    glane_sample_flag(&s, GLANE_F_INJECTED, 1);
    glane_sample_flag(&s, GLANE_F_SIMULATED, 1);
    CHECK_EQ_I(s.flags,
               GLANE_F_FIRST | GLANE_F_CALIBRATED |
               GLANE_F_INJECTED | GLANE_F_SIMULATED);
}

static void test_null_safety(void)
{
    /* Aucun crash sur pointeur NULL. */
    glane_sample_init(NULL, 1, GLANE_UNIT_C);
    CHECK_EQ_I(glane_sample_emit(NULL, 1, 1), -1);
    glane_sample_flag(NULL, GLANE_F_FIRST, 1);
    CHECK(1);  /* si on arrive ici, pas de segfault */
}

int main(void)
{
    test_size_and_layout();
    test_names();
    test_emit_seq_monotone();
    test_flags();
    test_null_safety();
    TEST_DONE();
}