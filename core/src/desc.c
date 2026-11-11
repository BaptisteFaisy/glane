/*
 * Glane — Copyright 2026 Baptiste Faisy. Licence Apache-2.0.
 *
 * desc.c — analyse de la description JSON d'un capteur (GLA-02,
 * CONTRACT.md §3). Pur : aucune allocation, aucune E/S, aucune horloge.
 * S'appuie sur le scanneur JSON interne (glane_jsonscan.h) et réutilise
 * glane_parse_decimal_scaled (calib.c) pour les nombres : zéro duplication.
 * La validation sémantique est dans desc_validate.c.
 */
#include "glane/glane_desc.h"
#include "glane/glane_jsonscan.h"

#include <limits.h>
#include <stddef.h>
#include <string.h>

/* Bits de présence des champs (suivi pendant l'analyse). */
enum {
    SEEN_ID       = 1u << 0,
    SEEN_NAME     = 1u << 1,
    SEEN_DRIVER   = 1u << 2,
    SEEN_BUSKIND  = 1u << 3,
    SEEN_PERIOD   = 1u << 4,
    SEEN_UNIT     = 1u << 5,
    SEEN_RANGE    = 1u << 6,
    SEEN_DEADLINE = 1u << 7,
    SEEN_STALE    = 1u << 8,
    SEEN_COEF     = 1u << 9,
    SEEN_WARN     = 1u << 10,
    SEEN_CRIT     = 1u << 11,
};
#define SEEN_REQUIRED (SEEN_ID | SEEN_NAME | SEEN_DRIVER | SEEN_BUSKIND | \
                       SEEN_PERIOD | SEEN_UNIT | SEEN_RANGE | SEEN_DEADLINE | \
                       SEEN_STALE)

#define NUMTOK_MAX 48
#define SMALLTOK_MAX 16

typedef struct {
    glane_desc *out;
    unsigned seen;
} dctx;

/* ------------------------------------------------------------------ */
/* Conversions de valeurs                                               */
/* ------------------------------------------------------------------ */

static int parse_u32_token(const char *tok, uint32_t *out,
                           glane_desc_error *err, const char *f, int line)
{
    const char *p = tok;
    if (*p == '+') p++;
    if (*p == '\0') {
        return json_fail(err, GLANE_DESC_ETYPE, f, line, "entier attendu");
    }
    uint64_t v = 0;
    for (; *p != '\0'; p++) {
        if (*p < '0' || *p > '9') {
            return json_fail(err, GLANE_DESC_ETYPE, f, line,
                             "entier attendu (caractère non numérique)");
        }
        v = v * 10u + (uint64_t)(*p - '0');
        if (v > 0xFFFFFFFFull) {
            return json_fail(err, GLANE_DESC_ERANGE, f, line,
                             "entier hors plage uint32");
        }
    }
    *out = (uint32_t)v;
    return 0;
}

static int parse_milli_value(const char *tok, int32_t *out,
                             glane_desc_error *err, const char *f, int line)
{
    int64_t v;
    int rc = glane_parse_decimal_scaled(tok, 3, &v);
    if (rc != 0) {
        return json_fail(err, GLANE_DESC_ETYPE, f, line, glane_calib_err(rc));
    }
    if (v > (int64_t)INT32_MAX || v < (int64_t)INT32_MIN) {
        return json_fail(err, GLANE_DESC_ERANGE, f, line,
                         "valeur milli hors int32");
    }
    *out = (int32_t)v;
    return 0;
}

static int parse_hex_or_dec(const char *s, uint32_t *out,
                            glane_desc_error *err, const char *f, int line)
{
    const char *p = s;
    int base = 10;
    if (p[0] == '0' && (p[1] == 'x' || p[1] == 'X')) {
        base = 16;
        p += 2;
    }
    if (*p == '\0') {
        return json_fail(err, GLANE_DESC_ETYPE, f, line, "adresse invalide");
    }
    uint64_t v = 0;
    for (; *p != '\0'; p++) {
        int d;
        char ch = *p;
        if (ch >= '0' && ch <= '9') d = ch - '0';
        else if (base == 16 && ch >= 'a' && ch <= 'f') d = ch - 'a' + 10;
        else if (base == 16 && ch >= 'A' && ch <= 'F') d = ch - 'A' + 10;
        else return json_fail(err, GLANE_DESC_ETYPE, f, line,
                              "caractère invalide dans l'adresse");
        v = v * (uint64_t)base + (uint64_t)d;
        if (v > 0xFFFFFFFFull) {
            return json_fail(err, GLANE_DESC_ERANGE, f, line,
                             "adresse hors plage");
        }
    }
    *out = (uint32_t)v;
    return 0;
}

static uint16_t unit_from_name(const char *s)
{
    if (strcmp(s, "C") == 0) return GLANE_UNIT_C;
    if (strcmp(s, "K") == 0) return GLANE_UNIT_K;
    if (strcmp(s, "V") == 0) return GLANE_UNIT_V;
    if (strcmp(s, "mA") == 0) return GLANE_UNIT_MA;
    if (strcmp(s, "deg/s") == 0) return GLANE_UNIT_DEG_S;
    if (strcmp(s, "g") == 0) return GLANE_UNIT_G;
    if (strcmp(s, "rpm") == 0) return GLANE_UNIT_RPM;
    if (strcmp(s, "%") == 0) return GLANE_UNIT_PCT;
    if (strcmp(s, "count") == 0) return GLANE_UNIT_COUNT;
    return 0;
}

static int32_t bus_from_name(const char *s)
{
    if (strcmp(s, "i2c") == 0) return GLANE_BUS_I2C;
    if (strcmp(s, "spi") == 0) return GLANE_BUS_SPI;
    if (strcmp(s, "gpio") == 0) return GLANE_BUS_GPIO;
    if (strcmp(s, "file") == 0) return GLANE_BUS_FILE;
    if (strcmp(s, "virtuel") == 0) return GLANE_BUS_VIRTUEL;
    return 0;
}

/* ------------------------------------------------------------------ */
/* Helpers de champ (factorisent « token -> champ + bit de présence »)  */
/* ------------------------------------------------------------------ */

static int field_u32(json_cursor *c, const char *key, uint32_t *dst,
                     dctx *ctx, unsigned bit, glane_desc_error *err)
{
    char tok[NUMTOK_MAX];
    if (json_number_token(c, tok, sizeof tok, err, key)) return err->code;
    if (parse_u32_token(tok, dst, err, key, c->line)) return err->code;
    ctx->seen |= bit;
    return 0;
}

static int field_str(json_cursor *c, const char *key, char *dst, size_t n,
                     dctx *ctx, unsigned bit, glane_desc_error *err)
{
    if (json_string(c, dst, n, err, key)) return err->code;
    ctx->seen |= bit;
    return 0;
}

static int field_milli(json_cursor *c, const char *key, int32_t *dst,
                       dctx *ctx, unsigned bit, glane_desc_error *err)
{
    char tok[NUMTOK_MAX];
    if (json_number_token(c, tok, sizeof tok, err, key)) return err->code;
    if (parse_milli_value(tok, dst, err, key, c->line)) return err->code;
    ctx->seen |= bit;
    return 0;
}

/* ------------------------------------------------------------------ */
/* Sous-objets : range, calib.coef, bus.addr                           */
/* ------------------------------------------------------------------ */

static int parse_range(json_cursor *c, glane_desc *o, glane_desc_error *err)
{
    if (json_expect(c, '[', err, "range")) return err->code;
    int32_t vals[2];
    for (int i = 0; i < 2; i++) {
        char tok[NUMTOK_MAX];
        if (json_number_token(c, tok, sizeof tok, err, "range"))
            return err->code;
        if (parse_milli_value(tok, &vals[i], err, "range", c->line))
            return err->code;
        if (i == 0) {
            json_ws(c);
            if (json_ch(c) != ',')
                return json_fail(err, GLANE_DESC_EJSON, "range", c->line,
                                 "',' attendu entre les 2 bornes");
            c->pos++;
        }
    }
    if (json_expect(c, ']', err, "range")) return err->code;
    o->range_min_milli = vals[0];
    o->range_max_milli = vals[1];
    return 0;
}

static int parse_coef_array(json_cursor *c, glane_calib *cal,
                            glane_desc_error *err)
{
    if (json_expect(c, '[', err, "calib.coef")) return err->code;
    json_ws(c);
    if (json_ch(c) == ']')
        return json_fail(err, GLANE_DESC_ECALIB, "calib.coef", c->line,
                         "tableau de coefficients vide");
    int n = 0;
    for (;;) {
        if (n > GLANE_CALIB_MAX_DEG)
            return json_fail(err, GLANE_DESC_ECALIB, "calib.coef", c->line,
                             "trop de coefficients (degré > 3)");
        char tok[NUMTOK_MAX];
        if (json_number_token(c, tok, sizeof tok, err, "calib.coef"))
            return err->code;
        int64_t v;
        int rc = glane_calib_parse_coef(tok, &v);
        if (rc != 0)
            return json_fail(err, GLANE_DESC_ECALIB, "calib.coef", c->line,
                             glane_calib_err(rc));
        cal->coef[n++] = v;
        json_ws(c);
        int d = json_ch(c);
        if (d == ',') { c->pos++; continue; }
        if (d == ']') { c->pos++; break; }
        return json_fail(err, GLANE_DESC_EJSON, "calib.coef", c->line,
                         "',' ou ']' attendu");
    }
    cal->deg = n - 1;
    return 0;
}

static int parse_bus_addr(json_cursor *c, uint32_t *out,
                          glane_desc_error *err, const char *key)
{
    json_ws(c);
    if (json_ch(c) == '"') {
        char s[GLANE_NAME_MAX];
        if (json_string(c, s, sizeof s, err, key)) return err->code;
        return parse_hex_or_dec(s, out, err, key, c->line);
    }
    char tok[NUMTOK_MAX];
    if (json_number_token(c, tok, sizeof tok, err, key)) return err->code;
    return parse_u32_token(tok, out, err, key, c->line);
}

/* ------------------------------------------------------------------ */
/* Gestionnaires de membres (un par objet JSON)                        */
/* ------------------------------------------------------------------ */

static int bus_member(void *vctx, const char *key, json_cursor *c,
                      glane_desc_error *err)
{
    dctx *ctx = (dctx *)vctx;
    glane_desc *o = ctx->out;
    if (strcmp(key, "kind") == 0) {
        char b[SMALLTOK_MAX];
        if (json_string(c, b, sizeof b, err, key)) return err->code;
        int32_t k = bus_from_name(b);
        if (k == 0)
            return json_fail(err, GLANE_DESC_EBUS, key, c->line,
                             "bus.kind inconnu");
        o->bus_kind = k;
        ctx->seen |= SEEN_BUSKIND;
        return 0;
    }
    if (strcmp(key, "dev") == 0)
        return field_str(c, key, o->bus_dev, sizeof o->bus_dev, ctx, 0, err);
    if (strcmp(key, "addr") == 0)
        return parse_bus_addr(c, &o->bus_addr, err, key);
    return json_skip_value(c, err, key);
}

static int calib_member(void *vctx, const char *key, json_cursor *c,
                        glane_desc_error *err)
{
    dctx *ctx = (dctx *)vctx;
    glane_calib *cal = &ctx->out->calib;
    if (strcmp(key, "kind") == 0) {
        char k[SMALLTOK_MAX];
        if (json_string(c, k, sizeof k, err, key)) return err->code;
        if (strcmp(k, "none") == 0) cal->kind = GLANE_CALIB_NONE;
        else if (strcmp(k, "poly") == 0) cal->kind = GLANE_CALIB_POLY;
        else return json_fail(err, GLANE_DESC_ECALIB, key, c->line,
                              "calib.kind inconnu (none|poly)");
        return 0;
    }
    if (strcmp(key, "coef") == 0) {
        int r = parse_coef_array(c, cal, err);
        if (r == 0) ctx->seen |= SEEN_COEF;
        return r;
    }
    return json_skip_value(c, err, key);
}

static int thr_member(void *vctx, const char *key, json_cursor *c,
                      glane_desc_error *err)
{
    dctx *ctx = (dctx *)vctx;
    glane_desc *o = ctx->out;
    if (strcmp(key, "warn") == 0)
        return field_milli(c, key, &o->warn_milli, ctx, SEEN_WARN, err);
    if (strcmp(key, "crit") == 0)
        return field_milli(c, key, &o->crit_milli, ctx, SEEN_CRIT, err);
    return json_skip_value(c, err, key);
}

static int top_member(void *vctx, const char *key, json_cursor *c,
                      glane_desc_error *err)
{
    dctx *ctx = (dctx *)vctx;
    glane_desc *o = ctx->out;
    if (strcmp(key, "id") == 0)
        return field_u32(c, key, &o->id, ctx, SEEN_ID, err);
    if (strcmp(key, "name") == 0)
        return field_str(c, key, o->name, sizeof o->name, ctx, SEEN_NAME, err);
    if (strcmp(key, "driver") == 0)
        return field_str(c, key, o->driver, sizeof o->driver, ctx,
                         SEEN_DRIVER, err);
    if (strcmp(key, "period_ms") == 0)
        return field_u32(c, key, &o->period_ms, ctx, SEEN_PERIOD, err);
    if (strcmp(key, "deadline_ms") == 0)
        return field_u32(c, key, &o->deadline_ms, ctx, SEEN_DEADLINE, err);
    if (strcmp(key, "stale_after_ms") == 0)
        return field_u32(c, key, &o->stale_after_ms, ctx, SEEN_STALE, err);
    if (strcmp(key, "unit") == 0) {
        char u[SMALLTOK_MAX];
        if (json_string(c, u, sizeof u, err, key)) return err->code;
        uint16_t code = unit_from_name(u);
        if (code == 0)
            return json_fail(err, GLANE_DESC_EUNIT, key, c->line,
                             "unité inconnue");
        o->unit = code;
        ctx->seen |= SEEN_UNIT;
        return 0;
    }
    if (strcmp(key, "range") == 0) {
        int r = parse_range(c, o, err);
        if (r == 0) ctx->seen |= SEEN_RANGE;
        return r;
    }
    if (strcmp(key, "bus") == 0)
        return json_parse_object(c, ctx, bus_member, err, "bus");
    if (strcmp(key, "calib") == 0) {
        int r = json_parse_object(c, ctx, calib_member, err, "calib");
        if (r != 0) return r;
        if (o->calib.kind == GLANE_CALIB_POLY && !(ctx->seen & SEEN_COEF))
            return json_fail(err, GLANE_DESC_ECALIB, "calib.coef", c->line,
                             "coef requis quand kind=poly");
        return 0;
    }
    if (strcmp(key, "thresholds") == 0)
        return json_parse_object(c, ctx, thr_member, err, "thresholds");
    return json_skip_value(c, err, key);  /* clé inconnue : ignorée */
}

/* ------------------------------------------------------------------ */
/* API publique : glane_desc_parse                                     */
/* ------------------------------------------------------------------ */

static const char *missing_field(unsigned seen)
{
    if (!(seen & SEEN_ID)) return "id";
    if (!(seen & SEEN_NAME)) return "name";
    if (!(seen & SEEN_DRIVER)) return "driver";
    if (!(seen & SEEN_BUSKIND)) return "bus.kind";
    if (!(seen & SEEN_PERIOD)) return "period_ms";
    if (!(seen & SEEN_UNIT)) return "unit";
    if (!(seen & SEEN_RANGE)) return "range";
    if (!(seen & SEEN_DEADLINE)) return "deadline_ms";
    if (!(seen & SEEN_STALE)) return "stale_after_ms";
    return "";
}

int glane_desc_parse(const char *json, size_t len, glane_desc *out,
                     glane_desc_error *err)
{
    if (json == NULL || out == NULL || err == NULL) {
        return GLANE_DESC_EFIELD;
    }
    memset(out, 0, sizeof *out);
    out->calib.kind = GLANE_CALIB_NONE;
    dctx ctx = { out, 0 };
    json_cursor c = { json, len, 0, 1 };

    int r = json_parse_object(&c, &ctx, top_member, err, "");
    if (r != 0) return r;
    json_ws(&c);
    if (c.pos != c.len)
        return json_fail(err, GLANE_DESC_EJSON, "", c.line,
                         "données après l'objet");
    if ((ctx.seen & SEEN_REQUIRED) != SEEN_REQUIRED)
        return json_fail(err, GLANE_DESC_EFIELD, missing_field(ctx.seen),
                         c.line, "champ obligatoire manquant");
    if ((ctx.seen & SEEN_WARN) && (ctx.seen & SEEN_CRIT) &&
        out->warn_milli > out->crit_milli)
        return json_fail(err, GLANE_DESC_ERANGE, "thresholds", c.line,
                         "warn doit être <= crit");
    return glane_desc_validate(out, err);
}