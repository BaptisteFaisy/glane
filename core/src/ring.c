/*
 * Glane — Copyright 2026 Baptiste Faisy. Licence Apache-2.0.
 *
 * ring.c — tampon circulaire de mesures (GLA-03, GLA-16).
 *
 * Aucune allocation : le stockage est fourni par l'appelant. Sert côté
 * abonné à détecter les pertes de `seq` et à rejouer dans l'ordre.
 */
#include "glane/glane_ring.h"

#include <stddef.h>

void glane_ring_init(glane_ring *r, glane_sample *buf, size_t cap)
{
    if (r == NULL) {
        return;
    }
    r->buf   = buf;
    r->cap   = (cap == 0) ? 0 : cap;
    r->head  = 0;
    r->count = 0;
}

int glane_ring_push(glane_ring *r, const glane_sample *s)
{
    if (r == NULL || s == NULL || r->buf == NULL || r->cap == 0) {
        return -1;
    }
    int overwrote = (r->count == r->cap) ? 1 : 0;
    r->buf[r->head] = *s;
    r->head = (r->head + 1) % r->cap;
    if (r->count < r->cap) {
        r->count++;
    }
    return overwrote;
}

/* Indice du i-ème élément le plus ancien (i=0 = plus ancien encore présent). */
static size_t ring_oldest_index(const glane_ring *r, size_t i)
{
    /* Les éléments occupent [head-count, head-1] (mod cap). */
    size_t base = (r->head + r->cap - r->count) % r->cap;
    return (base + i) % r->cap;
}

int glane_ring_peek(const glane_ring *r, size_t i, glane_sample *out)
{
    if (r == NULL || out == NULL || r->buf == NULL || i >= r->count) {
        return -1;
    }
    *out = r->buf[ring_oldest_index(r, i)];
    return 0;
}

size_t glane_ring_count(const glane_ring *r)
{
    if (r == NULL) {
        return 0;
    }
    return r->count;
}

int glane_ring_find_gap(const glane_ring *r, size_t *first_gap)
{
    if (r == NULL || r->buf == NULL || r->count < 2) {
        return -1;
    }
    for (size_t i = 0; i + 1 < r->count; i++) {
        uint32_t cur  = r->buf[ring_oldest_index(r, i)].seq;
        uint32_t next = r->buf[ring_oldest_index(r, i + 1)].seq;
        /* Consécutif attendu, wrap UINT32_MAX -> 0 inclus. */
        uint32_t expected = (uint32_t)(cur + 1u);
        if (next != expected) {
            if (first_gap != NULL) {
                *first_gap = i;
            }
            return (int)i;
        }
    }
    return -1;
}