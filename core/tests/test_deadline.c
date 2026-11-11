/*
 * Glane — Copyright 2026 Baptiste Faisy. Licence Apache-2.0.
 *
 * test_deadline.c — vérifie péremption et panne (GLA-06, GLA-07).
 */
#include "glane/glane_deadline.h"
#include "test_util.h"

static void test_quality_fresh_vs_stale(void)
{
    glane_deadline d;
    /* stale_after = 500 ms, fault après 3 absences. */
    glane_deadline_init(&d, 150, 500, 3);
    CHECK_EQ_I(d.stale_after_ns, 500LL * 1000000LL);
    CHECK_EQ_I(d.deadline_ns, 150LL * 1000000LL);
    CHECK_EQ_I(d.fault_after, 3u);

    /* Fraîche : age <= stale_after -> OK. */
    CHECK_EQ_I(glane_deadline_quality(&d, 0), GLANE_Q_OK);
    CHECK_EQ_I(glane_deadline_quality(&d, 100LL * 1000000LL), GLANE_Q_OK);
    CHECK_EQ_I(glane_deadline_quality(&d, 500LL * 1000000LL), GLANE_Q_OK);  /* frontière */
    /* Périmée : age > stale_after -> STALE. */
    CHECK_EQ_I(glane_deadline_quality(&d, 500LL * 1000000LL + 1), GLANE_Q_STALE);
    CHECK_EQ_I(glane_deadline_quality(&d, 600LL * 1000000LL), GLANE_Q_STALE);
}

static void test_missed_to_fault(void)
{
    glane_deadline d;
    glane_deadline_init(&d, 100, 500, 3);
    CHECK_EQ_I(d.missing, 0u);
    /* 1re absence : STALE (pas encore fault_after). */
    CHECK_EQ_I(glane_deadline_missed(&d), GLANE_Q_STALE);
    CHECK_EQ_I(d.missing, 1u);
    /* 2e absence : toujours STALE. */
    CHECK_EQ_I(glane_deadline_missed(&d), GLANE_Q_STALE);
    CHECK_EQ_I(d.missing, 2u);
    /* 3e absence : fault_after atteint -> FAULT. */
    CHECK_EQ_I(glane_deadline_missed(&d), GLANE_Q_FAULT);
    CHECK_EQ_I(d.missing, 3u);
    /* Au-delà : reste FAULT. */
    CHECK_EQ_I(glane_deadline_missed(&d), GLANE_Q_FAULT);
}

static void test_alive_resets(void)
{
    glane_deadline d;
    glane_deadline_init(&d, 100, 500, 2);
    glane_deadline_missed(&d);
    CHECK_EQ_I(d.missing, 1u);
    glane_deadline_alive(&d);
    CHECK_EQ_I(d.missing, 0u);
    /* Après reset, il faut à nouveau fault_after absences. */
    CHECK_EQ_I(glane_deadline_missed(&d), GLANE_Q_STALE);
    CHECK_EQ_I(glane_deadline_missed(&d), GLANE_Q_FAULT);
}

static void test_fault_after_one(void)
{
    glane_deadline d;
    /* fault_after = 1 : première absence -> FAULT direct. */
    glane_deadline_init(&d, 100, 500, 1);
    CHECK_EQ_I(glane_deadline_missed(&d), GLANE_Q_FAULT);
    /* fault_after = 0 normalisé à 1. */
    glane_deadline_init(&d, 100, 500, 0);
    CHECK_EQ_I(d.fault_after, 1u);
    CHECK_EQ_I(glane_deadline_missed(&d), GLANE_Q_FAULT);
}

static void test_null_safety(void)
{
    glane_deadline_init(NULL, 1, 1, 1);
    CHECK_EQ_I(glane_deadline_quality(NULL, 0), GLANE_Q_OK);
    CHECK_EQ_I(glane_deadline_missed(NULL), GLANE_Q_FAULT);
    glane_deadline_alive(NULL);
    CHECK(1);
}

int main(void)
{
    test_quality_fresh_vs_stale();
    test_missed_to_fault();
    test_alive_resets();
    test_fault_after_one();
    test_null_safety();
    TEST_DONE();
}