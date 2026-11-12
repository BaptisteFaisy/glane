/*
 * Glane — Copyright 2026 Baptiste Faisy. Licence Apache-2.0.
 *
 * fuzz_desc.c — harnais de fuzz déterministe du parseur de description
 * (GLA-17, §brief de fuzz). N'alimente QUE glane_desc_parse /
 * glane_desc_validate (pas le RNG de virtuel.c, pas d'autre module).
 *
 * - RNG xorshift64* à état passé explicitement (aucun état global mutable) ;
 * - N itérations, chaque entrée est un tampon de longueur 0..2048 ;
 * - ~10 % des entrées sont des descriptions valides (core/tests/valid/, les
 *   fichiers .json) mutées sur 1 ou 2 octets, pour explorer les chemins profonds ;
 * - toute lecture hors tampon / crash est signalée par ASan/UBSan : le binaire
 *   doit planter, c'est exactement la détection recherchée.
 *
 * Usage : fuzz_desc [N] [seed]   (défauts : N=100000, seed=0x9E3779B97F4A7C15)
 */
#include "glane/glane_desc.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_LEN        2048
#define DEFAULT_N      100000
#define DEFAULT_SEED   UINT64_C(0x9E3779B97F4A7C15)
#define MAX_SEEDS      16

/* Descriptions valides servant de graines au mode guidé. Miroir des fichiers
 * .json de core/tests/valid/ : on les lit au lancement (source unique, pas de
 * copie à maintenir). Lecture à l'exécution car `make fuzz` tourne à la
 * racine du dépôt ; hors dépôt, le mode guidé se désactive sans erreur.    */
static const char *const SEED_FILES[] = {
    "core/tests/valid/bus-current.json",
    "core/tests/valid/sim-ramp.json",
    "core/tests/valid/temp-chassis.json",
};

static unsigned char g_seeds[MAX_SEEDS][MAX_LEN];
static size_t        g_seed_len[MAX_SEEDS];
static size_t        g_nseeds;

/* xorshift64* — retourne le prochain mot 64 bits. */
static uint64_t rng_next(uint64_t *s)
{
    uint64_t x = *s;
    x ^= x >> 12;
    x ^= x << 25;
    x ^= x >> 27;
    *s = x;
    return x * UINT64_C(0x2545F4914F6CDD1D);
}

/* Entier uniforme dans [0, n) pour n > 0 (biais du modulo négligeable ici). */
static size_t rng_below(uint64_t *s, size_t n)
{
    return (size_t)(rng_next(s) % (uint64_t)n);
}

/* Charge les graines valides depuis le disque. Retourne le nombre chargé. */
static size_t load_seeds(void)
{
    size_t n = 0;
    size_t nfiles = sizeof(SEED_FILES) / sizeof(SEED_FILES[0]);
    for (size_t i = 0; i < nfiles && n < MAX_SEEDS; i++) {
        FILE *f = fopen(SEED_FILES[i], "rb");
        if (f == NULL)
            continue;
        size_t got = fread(g_seeds[n], 1, MAX_LEN, f);
        fclose(f);
        if (got == 0)
            continue;
        g_seed_len[n] = got;
        n++;
    }
    return n;
}

/* Remplit buf[0..len) d'octets pseudo-aléatoires. */
static void fill_random(uint64_t *s, unsigned char *buf, size_t len)
{
    for (size_t i = 0; i < len; i++)
        buf[i] = (unsigned char)(rng_next(s) >> 32);
}

/* Copie une graine valide et mute 1 ou 2 octets. Retourne la longueur. */
static size_t make_guided(uint64_t *s, unsigned char *buf)
{
    size_t k = rng_below(s, g_nseeds);
    size_t len = g_seed_len[k];
    memcpy(buf, g_seeds[k], len);
    int nmut = 1 + (int)(rng_next(s) & 1u); /* 1 ou 2 octets modifiés */
    for (int i = 0; i < nmut && len > 0; i++) {
        size_t pos = rng_below(s, len);
        buf[pos] = (unsigned char)(rng_next(s) >> 32);
    }
    return len;
}

/* Une itération de fuzz : tire une entrée, parse, valide si OK. */
static void fuzz_one(uint64_t *s, unsigned char *buf)
{
    size_t len;
    if (g_nseeds > 0 && (rng_next(s) % 10u) == 0u)
        len = make_guided(s, buf);
    else {
        len = (size_t)(rng_next(s) % (uint64_t)(MAX_LEN + 1));
        fill_random(s, buf, len);
    }

    glane_desc out;
    glane_desc_error err;
    memset(&out, 0, sizeof(out));
    memset(&err, 0, sizeof(err));

    int rc = glane_desc_parse((const char *)buf, len, &out, &err);
    if (rc == GLANE_DESC_OK)
        (void)glane_desc_validate(&out, &err);
    (void)rc;
}

int main(int argc, char **argv)
{
    long n = DEFAULT_N;
    uint64_t seed = DEFAULT_SEED;
    if (argc > 1)
        n = strtol(argv[1], NULL, 10);
    if (argc > 2)
        seed = strtoull(argv[2], NULL, 0);

    g_nseeds = load_seeds();

    static unsigned char buf[MAX_LEN]; /* statique : pas de gros cadre de pile */
    uint64_t s = seed;
    for (long i = 0; i < n; i++)
        fuzz_one(&s, buf);

    printf("OK %ld entrées fuzzées (seed %llu)\n",
           n, (unsigned long long)seed);
    return 0;
}