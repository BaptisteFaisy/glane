/*
 * Glane — Copyright 2026 Baptiste Faisy. Licence Apache-2.0.
 *
 * test_desc.c — vérifie l'analyse et la validation des descriptions (GLA-02).
 * Cas positifs (descriptions valides, champs extraits) + banque de 40
 * descriptions fautives (core/tests/faulty/), chacune devant produire le
 * code d'erreur attendu avec un message précis.
 */
#include "glane/glane_desc.h"
#include "test_util.h"

#include <stdio.h>
#include <string.h>

#define FILEBUF 4096

static int load_file(const char *path, char *buf, size_t bufsz, size_t *outlen)
{
    FILE *f = fopen(path, "rb");
    if (f == NULL) {
        return -1;
    }
    size_t n = fread(buf, 1, bufsz - 1, f);
    fclose(f);
    buf[n] = '\0';
    if (outlen != NULL) {
        *outlen = n;
    }
    return 0;
}

/* La description canonique (CONTRACT.md §3), tous champs présents. */
static const char *VALID_FULL =
    "{\"id\": 1, \"name\": \"temp-chassis\", \"driver\": \"tmp\","
    " \"bus\": {\"kind\": \"i2c\", \"dev\": \"/dev/i2c0\", \"addr\": \"0x48\"},"
    " \"period_ms\": 100, \"unit\": \"C\", \"range\": [-40.0, 125.0],"
    " \"calib\": {\"kind\": \"poly\", \"coef\": [0.0, 1.0]},"
    " \"thresholds\": {\"warn\": 85.0, \"crit\": 105.0},"
    " \"deadline_ms\": 150, \"stale_after_ms\": 500}";

static void test_valid_full(void)
{
    glane_desc d;
    glane_desc_error e;
    int rc = glane_desc_parse(VALID_FULL, strlen(VALID_FULL), &d, &e);
    CHECK_EQ_I(rc, GLANE_DESC_OK);
    CHECK_EQ_I(e.code, GLANE_DESC_OK);
    CHECK_EQ_I(d.id, 1u);
    CHECK_EQ_S(d.name, "temp-chassis");
    CHECK_EQ_S(d.driver, "tmp");
    CHECK_EQ_I(d.bus_kind, GLANE_BUS_I2C);
    CHECK_EQ_S(d.bus_dev, "/dev/i2c0");
    CHECK_EQ_I(d.bus_addr, 0x48u);
    CHECK_EQ_I(d.period_ms, 100u);
    CHECK_EQ_I(d.deadline_ms, 150u);
    CHECK_EQ_I(d.stale_after_ms, 500u);
    CHECK_EQ_I(d.unit, GLANE_UNIT_C);
    CHECK_EQ_I(d.range_min_milli, -40000);
    CHECK_EQ_I(d.range_max_milli, 125000);
    CHECK_EQ_I(d.warn_milli, 85000);
    CHECK_EQ_I(d.crit_milli, 105000);
    CHECK_EQ_I(d.calib.kind, GLANE_CALIB_POLY);
    CHECK_EQ_I(d.calib.deg, 1);
    CHECK_EQ_I(d.calib.coef[0], 0);
    CHECK_EQ_I(d.calib.coef[1], 1000000);  /* gain 1.0 échelle 1e6 */
}

static void test_valid_minimal(void)
{
    /* Sans calib ni thresholds : calib défaut NONE, seuils à 0. */
    const char *j =
        "{\"id\": 7, \"name\": \"sim\", \"driver\": \"virtuel\","
        " \"bus\": {\"kind\": \"virtuel\"}, \"period_ms\": 10,"
        " \"unit\": \"g\", \"range\": [-16.0, 16.0],"
        " \"deadline_ms\": 15, \"stale_after_ms\": 50}";
    glane_desc d;
    glane_desc_error e;
    int rc = glane_desc_parse(j, strlen(j), &d, &e);
    CHECK_EQ_I(rc, GLANE_DESC_OK);
    CHECK_EQ_I(d.id, 7u);
    CHECK_EQ_I(d.bus_kind, GLANE_BUS_VIRTUEL);
    CHECK_EQ_I(d.unit, GLANE_UNIT_G);
    CHECK_EQ_I(d.range_min_milli, -16000);
    CHECK_EQ_I(d.range_max_milli, 16000);
    CHECK_EQ_I(d.calib.kind, GLANE_CALIB_NONE);
    CHECK_EQ_I(d.warn_milli, 0);
    CHECK_EQ_I(d.crit_milli, 0);
}

static void test_valid_addr_decimal(void)
{
    /* bus.addr en entier décimal plutôt qu'en chaîne hexadécimale. */
    const char *j =
        "{\"id\": 2, \"name\": \"x\", \"driver\": \"d\","
        " \"bus\": {\"kind\": \"spi\", \"addr\": 104}, \"period_ms\": 5,"
        " \"unit\": \"V\", \"range\": [0.0, 3.3],"
        " \"deadline_ms\": 8, \"stale_after_ms\": 20}";
    glane_desc d;
    glane_desc_error e;
    CHECK_EQ_I(glane_desc_parse(j, strlen(j), &d, &e), GLANE_DESC_OK);
    CHECK_EQ_I(d.bus_addr, 104u);
    CHECK_EQ_I(d.bus_kind, GLANE_BUS_SPI);
}

static void test_valid_unknown_key_ignored(void)
{
    /* Une clé inconnue est ignorée (forward-compat), pas une erreur. */
    const char *j =
        "{\"id\": 3, \"name\": \"y\", \"driver\": \"d\", \"future\": {\"a\": [1,2]},"
        " \"bus\": {\"kind\": \"file\"}, \"period_ms\": 1000,"
        " \"unit\": \"K\", \"range\": [0.0, 100.0],"
        " \"deadline_ms\": 1500, \"stale_after_ms\": 5000, \"extra\": null}";
    glane_desc d;
    glane_desc_error e;
    CHECK_EQ_I(glane_desc_parse(j, strlen(j), &d, &e), GLANE_DESC_OK);
    CHECK_EQ_I(d.id, 3u);
    CHECK_EQ_I(d.unit, GLANE_UNIT_K);
}

static void test_valid_files(void)
{
    const char *names[3] = {
        "core/tests/valid/temp-chassis.json",
        "core/tests/valid/sim-ramp.json",
        "core/tests/valid/bus-current.json",
    };
    for (int i = 0; i < 3; i++) {
        char buf[FILEBUF];
        size_t n = 0;
        if (load_file(names[i], buf, sizeof buf, &n) != 0) {
            fprintf(stderr, "FAIL: cannot load %s\n", names[i]);
            g_fails++;
            continue;
        }
        glane_desc d;
        glane_desc_error e;
        int rc = glane_desc_parse(buf, n, &d, &e);
        CHECK_EQ_I(rc, GLANE_DESC_OK);
        CHECK(d.id > 0);
    }
}

/* Banque fautive : fichier -> code d'erreur attendu. */
typedef struct {
    const char *file;
    int code;
} fault_t;

static const fault_t faults[] = {
    {"01_empty.json", GLANE_DESC_EJSON},
    {"02_no_close_brace.json", GLANE_DESC_EJSON},
    {"03_array_top.json", GLANE_DESC_EJSON},
    {"04_bad_escape.json", GLANE_DESC_EJSON},
    {"05_unterminated_string.json", GLANE_DESC_EJSON},
    {"06_missing_colon.json", GLANE_DESC_EJSON},
    {"07_trailing_data.json", GLANE_DESC_EJSON},
    {"08_range_missing_comma.json", GLANE_DESC_EJSON},
    {"09_range_three_elems.json", GLANE_DESC_EJSON},
    {"10_coef_not_array.json", GLANE_DESC_EJSON},
    {"11_id_string.json", GLANE_DESC_ETYPE},
    {"12_period_string.json", GLANE_DESC_ETYPE},
    {"13_name_number.json", GLANE_DESC_ETYPE},
    {"14_unit_number.json", GLANE_DESC_ETYPE},
    {"15_bus_kind_number.json", GLANE_DESC_ETYPE},
    {"16_addr_bad_hex.json", GLANE_DESC_ETYPE},
    {"17_trailing_comma.json", GLANE_DESC_ETYPE},
    {"18_coef_string.json", GLANE_DESC_ETYPE},
    {"19_missing_id.json", GLANE_DESC_EFIELD},
    {"20_missing_name.json", GLANE_DESC_EFIELD},
    {"21_missing_driver.json", GLANE_DESC_EFIELD},
    {"22_missing_bus.json", GLANE_DESC_EFIELD},
    {"23_missing_period.json", GLANE_DESC_EFIELD},
    {"24_missing_unit.json", GLANE_DESC_EFIELD},
    {"25_missing_range.json", GLANE_DESC_EFIELD},
    {"26_missing_deadline.json", GLANE_DESC_EFIELD},
    {"27_missing_stale.json", GLANE_DESC_EFIELD},
    {"28_name_uppercase.json", GLANE_DESC_EFIELD},
    {"29_name_empty.json", GLANE_DESC_EFIELD},
    {"30_driver_space.json", GLANE_DESC_EFIELD},
    {"31_id_zero.json", GLANE_DESC_ERANGE},
    {"32_period_zero.json", GLANE_DESC_ERANGE},
    {"33_stale_zero.json", GLANE_DESC_ERANGE},
    {"34_range_inverted.json", GLANE_DESC_ERANGE},
    {"35_warn_gt_crit.json", GLANE_DESC_ERANGE},
    {"36_id_overflow.json", GLANE_DESC_ERANGE},
    {"37_unit_unknown.json", GLANE_DESC_EUNIT},
    {"38_bus_unknown.json", GLANE_DESC_EBUS},
    {"39_calib_kind_unknown.json", GLANE_DESC_ECALIB},
    {"40_coef_too_many.json", GLANE_DESC_ECALIB},
};

static void test_faulty_bank(void)
{
    size_t nfaults = sizeof faults / sizeof faults[0];
    CHECK_EQ_I(nfaults, 40u);  /* la banque compte bien 40 cas */
    for (size_t i = 0; i < nfaults; i++) {
        char path[256];
        snprintf(path, sizeof path, "core/tests/faulty/%s", faults[i].file);
        char buf[FILEBUF];
        size_t n = 0;
        if (load_file(path, buf, sizeof buf, &n) != 0) {
            fprintf(stderr, "FAIL: cannot load %s\n", path);
            g_fails++;
            continue;
        }
        glane_desc d;
        glane_desc_error e;
        memset(&e, 0, sizeof e);
        int rc = glane_desc_parse(buf, n, &d, &e);
        CHECK_EQ_I(rc, faults[i].code);
        CHECK(e.cause[0] != '\0');            /* message précis renseigné */
        CHECK(glane_desc_errstr(rc) != NULL);  /* message court disponible */
    }
}

static void test_null_args(void)
{
    glane_desc d;
    glane_desc_error e;
    CHECK(glane_desc_parse(NULL, 10, &d, &e) != GLANE_DESC_OK);
    CHECK(glane_desc_parse("{}", 2, NULL, &e) != GLANE_DESC_OK);
    CHECK(glane_desc_parse("{}", 2, &d, NULL) != GLANE_DESC_OK);
    CHECK(glane_desc_validate(NULL, &e) != GLANE_DESC_OK);
}

static void test_validate_direct(void)
{
    /* Validation directe d'un glane_desc construit à la main. */
    glane_desc d;
    glane_desc_error e;
    memset(&d, 0, sizeof d);
    d.id = 5;
    strcpy(d.name, "ok-name");
    strcpy(d.driver, "drv");
    d.bus_kind = GLANE_BUS_I2C;
    d.period_ms = 10;
    d.stale_after_ms = 50;
    d.range_min_milli = 0;
    d.range_max_milli = 100;
    d.unit = GLANE_UNIT_C;
    d.calib.kind = GLANE_CALIB_NONE;
    CHECK_EQ_I(glane_desc_validate(&d, &e), GLANE_DESC_OK);

    d.id = 0;  /* id invalide */
    CHECK_EQ_I(glane_desc_validate(&d, &e), GLANE_DESC_ERANGE);
    d.id = 5;
    d.period_ms = 0;  /* période nulle */
    CHECK_EQ_I(glane_desc_validate(&d, &e), GLANE_DESC_ERANGE);
    d.period_ms = 10;
    d.unit = 9999;  /* unité inconnue */
    CHECK_EQ_I(glane_desc_validate(&d, &e), GLANE_DESC_EUNIT);
    d.unit = GLANE_UNIT_C;
    d.bus_kind = 99;  /* bus hors plage */
    CHECK_EQ_I(glane_desc_validate(&d, &e), GLANE_DESC_EBUS);
}

int main(void)
{
    test_valid_full();
    test_valid_minimal();
    test_valid_addr_decimal();
    test_valid_unknown_key_ignored();
    test_valid_files();
    test_faulty_bank();
    test_null_args();
    test_validate_direct();
    TEST_DONE();
}