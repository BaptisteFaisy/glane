/*
 * Glane — Copyright 2026 Baptiste Faisy. Licence Apache-2.0.
 *
 * test_schema.c — vérifie la validation des noms publiés (GLA-08).
 */
#include "glane/glane_schema.h"
#include "test_util.h"
#include <stdint.h>

static void test_valid_names(void)
{
    CHECK_EQ_I(glane_schema_validate_name("temp-chassis", SIZE_MAX), GLANE_SCHEMA_OK);
    CHECK_EQ_I(glane_schema_validate_name("imu_0", SIZE_MAX), GLANE_SCHEMA_OK);
    CHECK_EQ_I(glane_schema_validate_name("a", SIZE_MAX), GLANE_SCHEMA_OK);
    CHECK_EQ_I(glane_schema_validate_name("sensor.name", SIZE_MAX), GLANE_SCHEMA_OK);
    CHECK_EQ_I(glane_schema_validate_name("x_y-z.0", SIZE_MAX), GLANE_SCHEMA_OK);
    /* 63 caractères = OK (limite haute). */
    char long63[64];
    for (int i = 0; i < 63; i++) {
        long63[i] = 'a';
    }
    long63[63] = '\0';
    CHECK_EQ_I(glane_schema_validate_name(long63, SIZE_MAX), GLANE_SCHEMA_OK);
}

static void test_invalid_names(void)
{
    CHECK_EQ_I(glane_schema_validate_name("", SIZE_MAX), GLANE_SCHEMA_EMPTY);
    CHECK_EQ_I(glane_schema_validate_name(NULL, SIZE_MAX), GLANE_SCHEMA_EMPTY);
    /* 64 caractères = trop long (limite >= 64). */
    char long64[65];
    for (int i = 0; i < 64; i++) {
        long64[i] = 'a';
    }
    long64[64] = '\0';
    CHECK_EQ_I(glane_schema_validate_name(long64, SIZE_MAX), GLANE_SCHEMA_TOO_LONG);
    /* Caractères invalides. */
    CHECK_EQ_I(glane_schema_validate_name("Temp", SIZE_MAX), GLANE_SCHEMA_BAD_CHAR);   /* majuscule */
    CHECK_EQ_I(glane_schema_validate_name("temp chassis", SIZE_MAX), GLANE_SCHEMA_BAD_CHAR); /* espace */
    CHECK_EQ_I(glane_schema_validate_name("temp/chassis", SIZE_MAX), GLANE_SCHEMA_BAD_CHAR); /* slash */
    CHECK_EQ_I(glane_schema_validate_name("temp$", SIZE_MAX), GLANE_SCHEMA_BAD_CHAR);
    /* Point initial / final. */
    CHECK_EQ_I(glane_schema_validate_name(".temp", SIZE_MAX), GLANE_SCHEMA_DOT);
    CHECK_EQ_I(glane_schema_validate_name("temp.", SIZE_MAX), GLANE_SCHEMA_DOT);
    CHECK_EQ_I(glane_schema_validate_name(".", SIZE_MAX), GLANE_SCHEMA_DOT);
}

static void test_explicit_len(void)
{
    /* len explicite (non NUL-terminé) : seuls les 4 premiers octets comptent. */
    const char *s = "tempXXX";
    CHECK_EQ_I(glane_schema_validate_name(s, 4), GLANE_SCHEMA_OK);
    CHECK_EQ_I(glane_schema_validate_name(s, 7), GLANE_SCHEMA_BAD_CHAR);  /* 'X' */
}

static void test_required_leaves(void)
{
    const char *req[6] = {"value", "quality", "seq", "ts_ns", "unit", "info"};
    for (int i = 0; i < 6; i++) {
        CHECK_EQ_I(glane_schema_validate_leaf(req[i], SIZE_MAX), GLANE_SCHEMA_OK);
        CHECK_EQ_I(glane_schema_is_required_leaf(req[i], SIZE_MAX), 1);
    }
    /* `command` est valide mais pas obligatoire. */
    CHECK_EQ_I(glane_schema_validate_leaf("command", SIZE_MAX), GLANE_SCHEMA_OK);
    CHECK_EQ_I(glane_schema_is_required_leaf("command", SIZE_MAX), 0);
}

static void test_invalid_leaves(void)
{
    CHECK_EQ_I(glane_schema_validate_leaf("", SIZE_MAX), GLANE_SCHEMA_EMPTY);
    CHECK_EQ_I(glane_schema_validate_leaf(NULL, SIZE_MAX), GLANE_SCHEMA_EMPTY);
    CHECK_EQ_I(glane_schema_validate_leaf("Value", SIZE_MAX), GLANE_SCHEMA_BAD_CHAR);
    CHECK_EQ_I(glane_schema_validate_leaf("unknown", SIZE_MAX), GLANE_SCHEMA_BAD_CHAR);
    CHECK_EQ_I(glane_schema_validate_leaf("foo bar", SIZE_MAX), GLANE_SCHEMA_BAD_CHAR);
    CHECK_EQ_I(glane_schema_is_required_leaf("unknown", SIZE_MAX), 0);
    CHECK_EQ_I(glane_schema_is_required_leaf("", SIZE_MAX), 0);
    CHECK_EQ_I(glane_schema_is_required_leaf(NULL, SIZE_MAX), 0);
}

static void test_errstr(void)
{
    CHECK(glane_schema_errstr(GLANE_SCHEMA_OK) != NULL);
    CHECK(glane_schema_errstr(GLANE_SCHEMA_EMPTY) != NULL);
    CHECK(glane_schema_errstr(GLANE_SCHEMA_TOO_LONG) != NULL);
    CHECK(glane_schema_errstr(GLANE_SCHEMA_BAD_CHAR) != NULL);
    CHECK(glane_schema_errstr(GLANE_SCHEMA_DOT) != NULL);
    CHECK(glane_schema_errstr(9999) != NULL);
    CHECK_EQ_S(glane_schema_errstr(GLANE_SCHEMA_OK), "ok");
}

int main(void)
{
    test_valid_names();
    test_invalid_names();
    test_explicit_len();
    test_required_leaves();
    test_invalid_leaves();
    test_errstr();
    TEST_DONE();
}