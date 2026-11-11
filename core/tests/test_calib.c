/*
 * Glane — Copyright 2026 Baptiste Faisy. Licence Apache-2.0.
 *
 * test_calib.c — vérifie la calibration entière (GLA-05).
 */
#include "glane/glane_calib.h"
#include "test_util.h"
#include <limits.h>

static glane_calib poly(int32_t kind, int32_t deg,
                        int64_t c0, int64_t c1, int64_t c2, int64_t c3)
{
    glane_calib c;
    c.kind = kind;
    c.deg  = deg;
    c.coef[0] = c0;
    c.coef[1] = c1;
    c.coef[2] = c2;
    c.coef[3] = c3;
    return c;
}

static void test_identity_and_offset(void)
{
    /* NONE = identité, quelle que soit l'entrée. */
    glane_calib none = poly(GLANE_CALIB_NONE, 0, 0, 0, 0, 0);
    CHECK_EQ_I(glane_calib_apply(&none, 0), 0);
    CHECK_EQ_I(glane_calib_apply(&none, 12345), 12345);
    CHECK_EQ_I(glane_calib_apply(&none, -777), -777);
    /* Décalage +5000 milli (= +5 unités) : POLY deg 1, a0=5000*1e6, gain 1.0.
     * Le terme constant a0 est en milli-unités (CONTRACT.md §2). */
    glane_calib off = poly(GLANE_CALIB_POLY, 1, 5000000000LL, 1000000LL, 0, 0);
    CHECK_EQ_I(glane_calib_apply(&off, 0), 5000);
    CHECK_EQ_I(glane_calib_apply(&off, 1000), 6000);
    CHECK_EQ_I(glane_calib_apply(&off, -1000), 4000);
}

static void test_linear(void)
{
    /* valeur = 2*x : coef[1]=2.0 -> 2000000. */
    glane_calib lin = poly(GLANE_CALIB_POLY, 1, 0, 2000000LL, 0, 0);
    CHECK_EQ_I(glane_calib_apply(&lin, 0), 0);
    CHECK_EQ_I(glane_calib_apply(&lin, 1000), 2000);
    CHECK_EQ_I(glane_calib_apply(&lin, -1000), -2000);
    CHECK_EQ_I(glane_calib_apply(&lin, 12345), 24690);
    /* valeur = x + 1000 milli : a0=1000*1e6, gain 1.0. */
    glane_calib lin2 = poly(GLANE_CALIB_POLY, 1, 1000000000LL, 1000000LL, 0, 0);
    CHECK_EQ_I(glane_calib_apply(&lin2, 0), 1000);
    CHECK_EQ_I(glane_calib_apply(&lin2, 500), 1500);
    /* gain fractionnaire 0.5 : coef[1]=500000. */
    glane_calib half = poly(GLANE_CALIB_POLY, 1, 0, 500000LL, 0, 0);
    CHECK_EQ_I(glane_calib_apply(&half, 1000), 500);
    CHECK_EQ_I(glane_calib_apply(&half, 1001), 501);   /* 500.5 -> 501 (demi à l'infini) */
}

static void test_rounding(void)
{
    /* gain 1/3 : coef[1]=333333 (0.333333). x=3 -> 0.999999 -> arrondi 1. */
    glane_calib third = poly(GLANE_CALIB_POLY, 1, 0, 333333LL, 0, 0);
    CHECK_EQ_I(glane_calib_apply(&third, 3), 1);
    /* x=6 -> 1.999998 -> 2. */
    CHECK_EQ_I(glane_calib_apply(&third, 6), 2);
    /* demi à l'infini : coef[1]=500000, x=1 -> 0.5 -> 1 (arrondi au +inf). */
    glane_calib half = poly(GLANE_CALIB_POLY, 1, 0, 500000LL, 0, 0);
    CHECK_EQ_I(glane_calib_apply(&half, 1), 1);
    /* négatif : coef[1]=-500000, x=1 -> -0.5 -> -1 (demi à l'infini = loin de 0). */
    glane_calib nhalf = poly(GLANE_CALIB_POLY, 1, 0, -500000LL, 0, 0);
    CHECK_EQ_I(glane_calib_apply(&nhalf, 1), -1);
    CHECK_EQ_I(glane_calib_apply(&nhalf, 3), -2);  /* -1.5 -> -2 */
}

static void test_quadratic_and_cubic(void)
{
    /* valeur = x^2 : coef[2]=1.0 -> 1000000. x=10 -> 100. */
    glane_calib sq = poly(GLANE_CALIB_POLY, 2, 0, 0, 1000000LL, 0);
    CHECK_EQ_I(glane_calib_apply(&sq, 0), 0);
    CHECK_EQ_I(glane_calib_apply(&sq, 10), 100);
    CHECK_EQ_I(glane_calib_apply(&sq, -10), 100);
    /* valeur = x^3 : coef[3]=1.0. x=10 -> 1000. */
    glane_calib cu = poly(GLANE_CALIB_POLY, 3, 0, 0, 0, 1000000LL);
    CHECK_EQ_I(glane_calib_apply(&cu, 0), 0);
    CHECK_EQ_I(glane_calib_apply(&cu, 10), 1000);
    CHECK_EQ_I(glane_calib_apply(&cu, -10), -1000);
    /* polynôme complet : 1 + 2x + 3x^2 + 4x^3, x=2 -> 1+4+12+32=49. */
    glane_calib full = poly(GLANE_CALIB_POLY, 3,
                            1000000LL, 2000000LL, 3000000LL, 4000000LL);
    CHECK_EQ_I(glane_calib_apply(&full, 2), 49);
    /* x=1 -> 1+2+3+4=10. */
    CHECK_EQ_I(glane_calib_apply(&full, 1), 10);
}

static void test_clamp(void)
{
    /* Un résultat énorme doit être clampé dans int32, pas déborder. */
    glane_calib big = poly(GLANE_CALIB_POLY, 3, 0, 0, 0, 1000000LL);
    /* x = 1 000 000 -> x^3 = 1e18, /1e6 = 1e12 -> clamp INT32_MAX. */
    CHECK_EQ_I(glane_calib_apply(&big, 1000000), INT32_MAX);
    CHECK_EQ_I(glane_calib_apply(&big, -1000000), INT32_MIN);
}

static void test_parse_coef(void)
{
    int64_t v;
    CHECK_EQ_I(glane_calib_parse_coef("0", &v), 0);
    CHECK_EQ_I(v, 0);
    CHECK_EQ_I(glane_calib_parse_coef("1.0", &v), 0);
    CHECK_EQ_I(v, 1000000);
    CHECK_EQ_I(glane_calib_parse_coef("-1.0", &v), 0);
    CHECK_EQ_I(v, -1000000);
    CHECK_EQ_I(glane_calib_parse_coef("0.5", &v), 0);
    CHECK_EQ_I(v, 500000);
    CHECK_EQ_I(glane_calib_parse_coef("2.5", &v), 0);
    CHECK_EQ_I(v, 2500000);
    CHECK_EQ_I(glane_calib_parse_coef("125", &v), 0);
    CHECK_EQ_I(v, 125000000);
    CHECK_EQ_I(glane_calib_parse_coef("-40.0", &v), 0);
    CHECK_EQ_I(v, -40000000);
    CHECK_EQ_I(glane_calib_parse_coef("0.000001", &v), 0);
    CHECK_EQ_I(v, 1);
    CHECK_EQ_I(glane_calib_parse_coef("1e-3", &v), 0);
    CHECK_EQ_I(v, 1000);
    CHECK_EQ_I(glane_calib_parse_coef("1E2", &v), 0);
    CHECK_EQ_I(v, 100000000);
    /* Erreurs attendues. */
    CHECK(glane_calib_parse_coef("", &v) != 0);
    CHECK(glane_calib_parse_coef("abc", &v) != 0);
    CHECK(glane_calib_parse_coef("1.2.3", &v) != 0);
    CHECK(glane_calib_parse_coef("12x", &v) != 0);
    CHECK(glane_calib_parse_coef(NULL, &v) != 0);
    CHECK(glane_calib_parse_coef("1", NULL) != 0);
    /* Message d'erreur non NULL. */
    CHECK(glane_calib_err(0) != NULL);
    CHECK(glane_calib_err(99) != NULL);
}

int main(void)
{
    test_identity_and_offset();
    test_linear();
    test_rounding();
    test_quadratic_and_cubic();
    test_clamp();
    test_parse_coef();
    TEST_DONE();
}