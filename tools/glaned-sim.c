/*
 * Glane — Copyright 2026 Baptiste Faisy. Licence Apache-2.0.
 *
 * glaned-sim.c — publisher virtuel hôte (GLA-04, idées B2/B3).
 *
 * Lit une description de capteur, configure le générateur synthétique
 * déterministe (core/virtuel.c), applique calibration + saturation +
 * péremption, et écrit un journal de mesures (CONTRACT.md §7) avec horodatage
 * source ET horodatage de livraison (latence injectable). Deux exécutions
 * avec les mêmes paramètres produisent le même journal octet pour octet.
 *
 * Usage : glaned-sim <desc.json> <steps> <waveform> <seed> <latency_ns> <out>
 *   waveform ∈ const|sine|ramp|triangle|noise
 * Ce n'est pas du core/ : les E/S sont autorisées ici.
 */
#include "glane/glane_calib.h"
#include "glane/glane_deadline.h"
#include "glane/glane_desc.h"
#include "glane/glane_sample.h"
#include "glane/glane_virtuel.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define READBUF 8192

static long read_file(const char *path, char *buf, long bufsz)
{
    FILE *f = fopen(path, "rb");
    if (f == NULL) {
        return -1;
    }
    long n = (long)fread(buf, 1, (size_t)(bufsz - 1), f);
    fclose(f);
    if (n < 0) {
        n = 0;
    }
    buf[n] = '\0';
    return n;
}

static int waveform_from(const char *s)
{
    if (strcmp(s, "const") == 0) return GLANE_WF_CONST;
    if (strcmp(s, "sine") == 0) return GLANE_WF_SINE;
    if (strcmp(s, "ramp") == 0) return GLANE_WF_RAMP;
    if (strcmp(s, "triangle") == 0) return GLANE_WF_TRIANGLE;
    if (strcmp(s, "noise") == 0) return GLANE_WF_NOISE;
    return -1;
}

/* Construit les drapeaux sous forme de chaîne lisible (ou "-"). */
static const char *flags_str(uint8_t f)
{
    static char b[64];
    b[0] = '\0';
    if (f & GLANE_F_FIRST) strcat(b, "first|");
    if (f & GLANE_F_CALIBRATED) strcat(b, "calibrated|");
    if (f & GLANE_F_INJECTED) strcat(b, "injected|");
    if (f & GLANE_F_SIMULATED) strcat(b, "simulated|");
    if (b[0] == '\0') return "-";
    b[strlen(b) - 1] = '\0';  /* retire le '|' final */
    return b;
}

static void write_line(FILE *o, const glane_desc *d, const glane_sample *s,
                       int64_t d_ns)
{
    fprintf(o,
            "{\"s\":\"%s\",\"id\":%u,\"seq\":%u,\"ts_ns\":%lld,\"d_ns\":%lld,"
            "\"v_m\":%d,\"u\":\"%s\",\"q\":\"%s\",\"f\":\"%s\"}\n",
            d->name, (unsigned)s->sensor_id, (unsigned)s->seq,
            (long long)s->ts_ns, (long long)d_ns, (int)s->value_milli,
            glane_unit_name(s->unit), glane_quality_name(s->quality),
            flags_str(s->flags));
}

static glane_virtuel_cfg build_cfg(const glane_desc *d, int wf, uint32_t seed)
{
    glane_virtuel_cfg c;
    memset(&c, 0, sizeof c);
    c.sensor_id = d->id;
    c.unit = d->unit;
    c.wf = wf;
    c.base_milli = (d->range_min_milli + d->range_max_milli) / 2;
    c.amp_milli = (d->range_max_milli - d->range_min_milli) / 4;
    c.period_steps = 64;
    c.drift_per_step = 0;
    c.seed = seed;
    c.sat_min_milli = d->range_min_milli;
    c.sat_max_milli = d->range_max_milli;
    return c;
}

/* Boucle de simulation : desc -> virtuel -> calib -> péremption -> journal.
 * Déterministe à (desc, wf, seed, steps, latency) fixés. */
static void run_sim(FILE *out, const glane_desc *d, int wf, uint32_t seed,
                    long steps, long long latency)
{
    glane_virtuel_cfg cfg = build_cfg(d, wf, seed);
    glane_virtuel v;
    glane_virtuel_init(&v, &cfg);
    glane_deadline dl;
    glane_deadline_init(&dl, d->deadline_ms, d->stale_after_ms, 3);
    int64_t period_ns = (int64_t)d->period_ms * 1000000LL;
    for (long i = 0; i < steps; i++) {
        glane_sample s;
        int64_t ts = i * period_ns;
        glane_virtuel_next(&v, ts, &s);
        if (d->calib.kind != GLANE_CALIB_NONE) {
            s.value_milli = glane_calib_apply(&d->calib, s.value_milli);
            s.flags |= GLANE_F_CALIBRATED;
        }
        if (s.quality == GLANE_Q_OK &&
            glane_deadline_quality(&dl, latency) == GLANE_Q_STALE) {
            s.quality = GLANE_Q_STALE;
        }
        write_line(out, d, &s, ts + latency);
    }
}

int main(int argc, char **argv)
{
    if (argc != 7) {
        fprintf(stderr,
                "usage: %s <desc.json> <steps> <waveform> <seed> "
                "<latency_ns> <out.journal>\n", argv[0]);
        return 2;
    }
    long steps = strtol(argv[2], NULL, 10);
    int wf = waveform_from(argv[3]);
    uint32_t seed = (uint32_t)strtoul(argv[4], NULL, 10);
    long long latency = strtoll(argv[5], NULL, 10);
    if (wf < 0 || steps <= 0) {
        fprintf(stderr, "waveform ou steps invalide\n");
        return 2;
    }
    char buf[READBUF];
    long n = read_file(argv[1], buf, sizeof buf);
    if (n < 0) {
        fprintf(stderr, "lecture impossible: %s\n", argv[1]);
        return 2;
    }
    glane_desc d;
    glane_desc_error e;
    if (glane_desc_parse(buf, (size_t)n, &d, &e) != GLANE_DESC_OK) {
        fprintf(stderr, "description invalide: champ=%s ligne=%d: %s\n",
                e.field, e.line, e.cause);
        return 1;
    }
    FILE *out = fopen(argv[6], "wb");
    if (out == NULL) {
        fprintf(stderr, "ouverture impossible: %s\n", argv[6]);
        return 2;
    }
    run_sim(out, &d, wf, seed, steps, latency);
    fclose(out);
    return 0;
}