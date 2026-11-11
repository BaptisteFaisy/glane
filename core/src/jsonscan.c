/*
 * Glane — Copyright 2026 Baptiste Faisy. Licence Apache-2.0.
 *
 * jsonscan.c — micro-scanneur JSON interne (GLA-02). Voir glane_jsonscan.h.
 * Pur : aucune allocation, aucune E/S, aucune horloge, aucun état global.
 */
#include "glane/glane_jsonscan.h"

#include <string.h>

int json_fail(glane_desc_error *err, int code, const char *field,
              int line, const char *cause)
{
    if (err != NULL) {
        err->code = code;
        strncpy(err->field, field ? field : "", GLANE_NAME_MAX - 1);
        err->field[GLANE_NAME_MAX - 1] = '\0';
        err->line = line;
        strncpy(err->cause, cause ? cause : "", GLANE_DESC_ERR_MAX - 1);
        err->cause[GLANE_DESC_ERR_MAX - 1] = '\0';
    }
    return code;
}

void json_ws(json_cursor *c)
{
    while (c->pos < c->len) {
        char ch = c->s[c->pos];
        if (ch == ' ' || ch == '\t' || ch == '\r') {
            c->pos++;
        } else if (ch == '\n') {
            c->pos++;
            c->line++;
        } else {
            break;
        }
    }
}

int json_ch(json_cursor *c)
{
    return (c->pos < c->len) ? (unsigned char)c->s[c->pos] : -1;
}

int json_expect(json_cursor *c, char ch, glane_desc_error *err, const char *f)
{
    json_ws(c);
    if (json_ch(c) != (unsigned char)ch) {
        return json_fail(err, GLANE_DESC_EJSON, f, c->line,
                         "caractère structurel attendu");
    }
    c->pos++;
    return 0;
}

int json_string(json_cursor *c, char *out, size_t outmax,
                glane_desc_error *err, const char *f)
{
    json_ws(c);
    if (json_ch(c) != '"') {
        return json_fail(err, GLANE_DESC_ETYPE, f, c->line, "chaîne attendue");
    }
    c->pos++;
    size_t n = 0;
    while (c->pos < c->len) {
        char ch = c->s[c->pos];
        if (ch == '"') {
            c->pos++;
            out[n] = '\0';
            return 0;
        }
        if (ch == '\\') {
            c->pos++;
            if (c->pos >= c->len) break;
            char e = c->s[c->pos++];
            char d;
            switch (e) {
            case '"': d = '"'; break;  case '\\': d = '\\'; break;
            case '/': d = '/'; break;  case 'n': d = '\n'; break;
            case 't': d = '\t'; break; case 'r': d = '\r'; break;
            case 'b': d = '\b'; break; case 'f': d = '\f'; break;
            default:
                return json_fail(err, GLANE_DESC_EJSON, f, c->line,
                                 "échappement invalide");
            }
            if (n + 1 >= outmax) {
                return json_fail(err, GLANE_DESC_ERANGE, f, c->line,
                                 "chaîne trop longue");
            }
            out[n++] = d;
            continue;
        }
        if ((unsigned char)ch < 0x20) {
            return json_fail(err, GLANE_DESC_EJSON, f, c->line,
                             "caractère de contrôle dans une chaîne");
        }
        if (n + 1 >= outmax) {
            return json_fail(err, GLANE_DESC_ERANGE, f, c->line,
                             "chaîne trop longue");
        }
        out[n++] = ch;
        c->pos++;
    }
    return json_fail(err, GLANE_DESC_EJSON, f, c->line, "chaîne non terminée");
}

int json_number_token(json_cursor *c, char *buf, size_t bufmax,
                      glane_desc_error *err, const char *f)
{
    json_ws(c);
    size_t n = 0;
    while (c->pos < c->len) {
        char ch = c->s[c->pos];
        int isnum = (ch >= '0' && ch <= '9') || ch == '.' ||
                    ch == 'e' || ch == 'E' || ch == '+' || ch == '-';
        if (!isnum) break;
        if (n + 1 >= bufmax) {
            return json_fail(err, GLANE_DESC_ERANGE, f, c->line,
                             "nombre trop long");
        }
        buf[n++] = ch;
        c->pos++;
    }
    if (n == 0) {
        return json_fail(err, GLANE_DESC_ETYPE, f, c->line, "nombre attendu");
    }
    buf[n] = '\0';
    return 0;
}

static int skip_atom(json_cursor *c, glane_desc_error *err, const char *f)
{
    size_t start = c->pos;
    while (c->pos < c->len) {
        char ch = c->s[c->pos];
        if (ch == ',' || ch == '}' || ch == ']' || ch == ' ' ||
            ch == '\t' || ch == '\r' || ch == '\n') {
            break;
        }
        c->pos++;
    }
    if (c->pos == start) {
        return json_fail(err, GLANE_DESC_EJSON, f, c->line, "valeur attendue");
    }
    return 0;
}

int json_skip_value(json_cursor *c, glane_desc_error *err, const char *f)
{
    json_ws(c);
    int ch = json_ch(c);
    if (ch == '"') {
        char tmp[GLANE_NAME_MAX];
        return json_string(c, tmp, sizeof tmp, err, f);
    }
    if (ch == '{' || ch == '[') {
        char close = (ch == '{') ? '}' : ']';
        c->pos++;
        json_ws(c);
        if (json_ch(c) == close) {
            c->pos++;
            return 0;
        }
        for (;;) {
            if (ch == '{') {
                char k[GLANE_NAME_MAX];
                if (json_string(c, k, sizeof k, err, f)) return err->code;
                if (json_expect(c, ':', err, f)) return err->code;
            }
            if (json_skip_value(c, err, f)) return err->code;
            json_ws(c);
            int d = json_ch(c);
            if (d == ',') { c->pos++; continue; }
            if (d == close) { c->pos++; return 0; }
            return json_fail(err, GLANE_DESC_EJSON, f, c->line,
                             "séparateur attendu");
        }
    }
    return skip_atom(c, err, f);
}

int json_parse_object(json_cursor *c, void *ctx, json_member_fn fn,
                      glane_desc_error *err, const char *obj)
{
    if (json_expect(c, '{', err, obj)) return err->code;
    json_ws(c);
    if (json_ch(c) == '}') { c->pos++; return 0; }
    for (;;) {
        char key[GLANE_NAME_MAX];
        if (json_string(c, key, sizeof key, err, obj)) return err->code;
        if (json_expect(c, ':', err, key)) return err->code;
        if (fn(ctx, key, c, err)) return err->code;
        json_ws(c);
        int d = json_ch(c);
        if (d == ',') { c->pos++; continue; }
        if (d == '}') { c->pos++; return 0; }
        return json_fail(err, GLANE_DESC_EJSON, obj, c->line,
                         "',' ou '}' attendu");
    }
}