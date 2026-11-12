/*
 * Glane — Copyright 2026 Baptiste Faisy. Licence Apache-2.0.
 *
 * test_selftest.c — vérifie que le selftest sur vecteurs de référence passe
 * (idée A21). Si un vecteur de référence diverge du code, le bitset est non
 * nul et ce test échoue — c'est la garde qui protège le selftest lui-même.
 */
#include "glane/glane_selftest.h"
#include "test_util.h"

int main(void)
{
    uint32_t fail = glane_selftest();
    CHECK_EQ_I(fail, 0u);  /* tous les sous-systèmes verts */
    CHECK(fail == 0);
    /* La version des vecteurs est figée et positive. */
    CHECK(GLANE_SELFTEST_VECTORS_VERSION >= 1);
    /* Aucun bit individuel ne doit être levé. */
    CHECK_EQ_I(fail & GLANE_SELFTEST_CALIB, 0u);
    CHECK_EQ_I(fail & GLANE_SELFTEST_SAMPLE, 0u);
    CHECK_EQ_I(fail & GLANE_SELFTEST_VIRTUEL, 0u);
    CHECK_EQ_I(fail & GLANE_SELFTEST_DEADLINE, 0u);
    CHECK_EQ_I(fail & GLANE_SELFTEST_SATURATE, 0u);
    TEST_DONE();
}