/*
 * Glane — Copyright 2026 Baptiste Faisy. Licence Apache-2.0.
 *
 * glaned.c — le publisher (binaire cible QNX, GLA-12/13).
 *
 * Lit une description de capteur, produit les mesures (générateur virtuel
 * déterministe tant qu'il n'y a pas de matériel), applique calibration et
 * péremption, et les publie via la couche glane_publish (objets
 * /dev/sensors/<name> sur QNX ; repli stdout ailleurs).
 *
 * NOTE : la boucle desc→virtuel→calib est partagée avec tools/glaned-sim
 * (l'outil hôte qui écrit le journal + latence pour glat). Elle sera factorisée
 * dans publish/vpub quand le port QNX sera terminé — gardé séparé tant que la
 * couche publication QNX est à l'état d'échafaudage.
 */
#include "glane/glane_calib.h"
#include "glane/glane_deadline.h"
#include "glane/glane_desc.h"
#include "glane/glane_virtuel.h"
#include "glane_publish.h"

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

static glane_virtuel_cfg cfg_from_desc(const glane_desc *d, int wf, uint32_t seed)
{
    glane_virtuel_cfg c;
    memset(&c, 0, sizeof c);
    c.sensor_id = d->id;
    c.unit = d->unit;
    c.wf = wf;
    c.base_milli = (d->range_min_milli + d->range_max_milli) / 2;
    c.amp_milli = (d->range_max_milli - d->range_min_milli) / 4;
    c.period_steps = 64;
    c.seed = seed;
    c.sat_min_milli = d->range_min_milli;
    c.sat_max_milli = d->range_max_milli;
    return c;
}

/* Boucle de publication : desc -> virtuel -> calib -> péremption -> publish. */
static int run_publisher(const glane_desc *d, int wf, uint32_t seed, long steps)
{
    glane_virtuel_cfg cfg = cfg_from_desc(d, wf, seed);
    glane_virtuel v;
    glane_virtuel_init(&v, &cfg);
    glane_deadline dl;
    glane_deadline_init(&dl, d->deadline_ms, d->stale_after_ms, 3);
    glane_publisher pub;
    if (glane_publish_open(&pub, d) != 0) {
        fprintf(stderr, "ouverture de la publication impossible\n");
        return 1;
    }
    int64_t period_ns = (int64_t)d->period_ms * 1000000LL;
    for (long i = 0; i < steps; i++) {
        glane_sample s;
        glane_virtuel_next(&v, i * period_ns, &s);
        if (d->calib.kind != GLANE_CALIB_NONE) {
            s.value_milli = glane_calib_apply(&d->calib, s.value_milli);
            s.flags |= GLANE_F_CALIBRATED;
        }
        if (glane_deadline_quality(&dl, 0) == GLANE_Q_STALE) {
            s.quality = GLANE_Q_STALE;
        }
        glane_publish_sample(&pub, &s);
    }
    glane_publish_close(&pub);
    fprintf(stderr, "glaned: %lu mesures publiées pour '%s'\n",
            pub.published, d->name);
    return 0;
}

int main(int argc, char **argv)
{
    if (argc < 5) {
        fprintf(stderr, "usage: %s <desc.json> <steps> <waveform 0-4> <seed>\n",
                argv[0]);
        return 2;
    }
    char buf[READBUF];
    long n = read_file(argv[1], buf, sizeof buf);
    if (n < 0) {
        fprintf(stderr, "description illisible: %s\n", argv[1]);
        return 2;
    }
    glane_desc d;
    glane_desc_error e;
    if (glane_desc_parse(buf, (size_t)n, &d, &e) != GLANE_DESC_OK) {
        fprintf(stderr, "description invalide: champ=%s ligne=%d: %s\n",
                e.field, e.line, e.cause);
        return 1;
    }
    long steps = strtol(argv[2], NULL, 10);
    int wf = atoi(argv[3]);
    uint32_t seed = (uint32_t)strtoul(argv[4], NULL, 10);
    if (steps <= 0 || wf < 0 || wf > 4) {
        fprintf(stderr, "steps ou waveform invalide\n");
        return 2;
    }
    return run_publisher(&d, wf, seed, steps);
}