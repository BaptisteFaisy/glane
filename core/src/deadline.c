/*
 * Glane — Copyright 2026 Baptiste Faisy. Licence Apache-2.0.
 *
 * deadline.c — péremption et panne (GLA-06, GLA-07).
 *
 * Une mesure trop vieille est publiée STALE au lieu d'être jetée ; un capteur
 * muet passe FAULT après N périodes manquantes. Aucune suppression silencieuse.
 */
#include "glane/glane_deadline.h"

#include <stddef.h>

#define NS_PER_MS 1000000LL

void glane_deadline_init(glane_deadline *d, uint32_t deadline_ms,
                         uint32_t stale_after_ms, uint32_t miss_before_fault)
{
    if (d == NULL) {
        return;
    }
    d->deadline_ns    = (int64_t)deadline_ms * NS_PER_MS;
    d->stale_after_ns = (int64_t)stale_after_ms * NS_PER_MS;
    d->fault_after    = (miss_before_fault == 0) ? 1 : miss_before_fault;
    d->missing        = 0;
}

uint8_t glane_deadline_quality(const glane_deadline *d, int64_t age_ns)
{
    if (d == NULL) {
        return GLANE_Q_OK;
    }
    /* age_ns <= stale_after_ns -> fraîche ; sinon périmée (STALE). */
    if (age_ns <= d->stale_after_ns) {
        return GLANE_Q_OK;
    }
    return GLANE_Q_STALE;
}

uint8_t glane_deadline_missed(glane_deadline *d)
{
    if (d == NULL) {
        return GLANE_Q_FAULT;
    }
    d->missing++;
    if (d->missing >= d->fault_after) {
        return GLANE_Q_FAULT;
    }
    return GLANE_Q_STALE;
}

void glane_deadline_alive(glane_deadline *d)
{
    if (d != NULL) {
        d->missing = 0;
    }
}