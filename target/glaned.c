/*
 * Glane — Copyright 2026 Baptiste Faisy. Licence Apache-2.0.
 *
 * glaned.c — le publisher multi-capteurs (binaire cible QNX, GLA-12/13).
 *
 * Lit PLUSIEURS descriptions de capteur, produit les mesures via le générateur
 * virtuel déterministe de chacun (à la cadence déclarée par chaque description),
 * applique calibration et péremption, publie via glane_publish, et MESURE la
 * latence réelle de publication (ClockTime dans QNX) -> p50/p99/p99.9/max.
 *
 * Usage : glaned <pas> <desc1.json> [desc2.json ...]
 *         glaned -- <descs...> ; pas = durée totale en ticks de 10 ms.
 */
#ifdef __QNX__
#define _QNX_SOURCE 1
#include <sys/neutrino.h>
#include <time.h>
#else
#define _POSIX_C_SOURCE 200809L
#include <time.h>
#endif

#include "glane/glane_calib.h"
#include "glane/glane_deadline.h"
#include "glane/glane_desc.h"
#include "glane/glane_virtuel.h"
#include "glane_publish.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define READBUF 8192
#define MAX_SENSORS 8
#define TICK_MS 10
#define MAX_LAT 8192

typedef struct {
    glane_desc desc;
    glane_virtuel virt;
    glane_deadline dl;
    glane_publisher pub;
} sensor_t;

static sensor_t sensors[MAX_SENSORS];
static int nsensors;

static long long lat_vals[MAX_LAT];
static int lat_count;

/* Pause d'un pas (10 ms) — utilisée UNIQUEMENT en mode infini.
 *
 * Pourquoi : sans pause, la boucle « pas » tourne aussi vite que l'invité
 * émulé le permet (CPU saturé, ~8 000 pas/s mesurés). Le pas doit suivre le
 * temps réel, sinon `period_ms` ne veut rien dire et la démo sature la
 * machine. En mode fini (rejeu, captures), aucune pause : le rejeu doit être
 * instantané et déterministe. */
static void tick_pause(void)
{
    struct timespec ts;
    ts.tv_sec = 0;
    ts.tv_nsec = 10000000L;   /* 10 ms */
    nanosleep(&ts, NULL);
}

/* Horloge monotone : ClockTime (QNX, 3 args) ou clock_gettime (hôte). */
static long long now_ns(void)
{
#ifdef __QNX__
    uint64_t t = 0;
    ClockTime(CLOCK_MONOTONIC, NULL, &t);
    return (long long)t;
#else
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (long long)ts.tv_sec * 1000000000LL + ts.tv_nsec;
#endif
}

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

static glane_virtuel_cfg cfg_from_desc(const glane_desc *d, int idx)
{
    glane_virtuel_cfg c;
    memset(&c, 0, sizeof c);
    c.sensor_id = d->id;
    c.unit = d->unit;
    c.wf = (idx + 1) % 4;          /* sinusoïde, dent de scie, triangle… varié */
    c.base_milli = (d->range_min_milli + d->range_max_milli) / 2;
    c.amp_milli = (d->range_max_milli - d->range_min_milli) / 4;
    c.period_steps = 64;
    c.seed = 1000u + (uint32_t)idx;  /* graine distincte par capteur         */
    c.sat_min_milli = d->range_min_milli;
    c.sat_max_milli = d->range_max_milli;
    return c;
}

/* Charge un capteur : description + générateur + publication. */
static int load_sensor(const char *path, int idx)
{
    if (nsensors >= MAX_SENSORS) {
        return -1;
    }
    char buf[READBUF];
    long n = read_file(path, buf, sizeof buf);
    if (n < 0) {
        fprintf(stderr, "description illisible: %s\n", path);
        return -1;
    }
    sensor_t *s = &sensors[nsensors];
    glane_desc_error e;
    if (glane_desc_parse(buf, (size_t)n, &s->desc, &e) != GLANE_DESC_OK) {
        fprintf(stderr, "description invalide %s: %s (ligne %d)\n",
                path, e.cause, e.line);
        return -1;
    }
    glane_virtuel_cfg cfg = cfg_from_desc(&s->desc, idx);
    glane_virtuel_init(&s->virt, &cfg);
    glane_deadline_init(&s->dl, s->desc.deadline_ms, s->desc.stale_after_ms, 3);
    glane_publish_open(&s->pub, &s->desc);
    nsensors++;
    return 0;
}

static long long percentile(long long *v, int n, int p)
{
    int rank = (p * n + 99) / 100;
    if (rank < 1) rank = 1;
    if (rank > n) rank = n;
    return v[rank - 1];
}

static int cmp_ll(const void *a, const void *b)
{
    long long x = *(const long long *)a, y = *(const long long *)b;
    return (x > y) - (x < y);
}

static void report_latency(void)
{
    qsort(lat_vals, (size_t)lat_count, sizeof(long long), cmp_ll);
    printf("glane-latence: %d publications, p50=%lld ns, p99=%lld ns, "
           "p99.9=%lld ns, max=%lld ns\n",
           lat_count, percentile(lat_vals, lat_count, 50),
           percentile(lat_vals, lat_count, 99),
           percentile(lat_vals, lat_count, 999),
           lat_vals[lat_count - 1]);
}

/* Battement de coeur du mode infini : une ligne toutes les 100 pas (1 s). */
static void heartbeat(long step)
{
    unsigned long total = 0;
    for (int s = 0; s < nsensors; s++) {
        total += (unsigned long)sensors[s].pub.published;
    }
    fprintf(stderr, "glaned: mode infini — %ld pas, %lu publications\n",
            step + 1, total);
}

/* Produit puis publie une mesure d'un capteur. */
static void step_sensor(sensor_t *s, int64_t ts_ns)
{
    glane_sample m;
    glane_virtuel_next(&s->virt, ts_ns, &m);
    if (s->desc.calib.kind != GLANE_CALIB_NONE) {
        m.value_milli = glane_calib_apply(&s->desc.calib, m.value_milli);
        m.flags |= GLANE_F_CALIBRATED;
    }
    long long t0 = now_ns();
    glane_publish_sample(&s->pub, &m);
    long long t1 = now_ns();
    if (lat_count < MAX_LAT) {
        lat_vals[lat_count++] = t1 - t0;
    }
}

int main(int argc, char **argv)
{
    if (argc < 3) {
        fprintf(stderr,
                "usage: %s <pas> <desc1.json> [desc2.json ...]\n"
                "       pas = 0 : mode infini (publie tant que le processus vit)\n",
                argv[0]);
        return 2;
    }
    long steps = strtol(argv[1], NULL, 10);
    if (steps < 0) {
        fprintf(stderr, "pas invalide: %s\n", argv[1]);
        return 2;
    }
    for (int i = 2; i < argc && nsensors < MAX_SENSORS; i++) {
        load_sensor(argv[i], nsensors);
    }
    if (nsensors == 0) {
        fprintf(stderr, "aucun capteur valide\n");
        return 2;
    }
    if (steps == 0) {
        fprintf(stderr, "glaned: mode infini (pas=0) — publication continue\n");
    }
    for (long step = 0; steps == 0 || step < steps; step++) {
        int64_t ts_ns = (int64_t)step * TICK_MS * 1000000LL;
        for (int s = 0; s < nsensors; s++) {
            long period_ticks = sensors[s].desc.period_ms / TICK_MS;
            if (period_ticks < 1) period_ticks = 1;
            if (step % period_ticks != 0) {
                continue;
            }
            step_sensor(&sensors[s], ts_ns);
        }
        if (steps == 0) {
            if ((step % 100) == 99) {
                heartbeat(step);
            }
            tick_pause();
        }
    }
    for (int s = 0; s < nsensors; s++) {
        glane_publish_close(&sensors[s].pub);
        fprintf(stderr, "glaned: %lu mesures publiées pour '%s'\n",
                sensors[s].pub.published, sensors[s].desc.name);
    }
    report_latency();
    return 0;
}