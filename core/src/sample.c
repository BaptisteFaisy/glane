/*
 * Glane — Copyright 2026 Baptiste Faisy. Licence Apache-2.0.
 *
 * sample.c — construction, séquence monotone, qualité (GLA-03).
 *
 * La mesure est un POD de 24 octets (CONTRACT.md §1). Ce module fournit les
 * helpers purs pour l'initialiser, l'avancer (seq monotone, jamais de retour
 * en arrière de l'horodatage) et gérer ses drapeaux.
 */
#include "glane/glane_sample.h"

#include <stddef.h>

const char *glane_unit_name(uint16_t unit)
{
    switch (unit) {
    case GLANE_UNIT_C:     return "C";
    case GLANE_UNIT_K:     return "K";
    case GLANE_UNIT_V:     return "V";
    case GLANE_UNIT_MA:    return "mA";
    case GLANE_UNIT_DEG_S: return "deg/s";
    case GLANE_UNIT_G:     return "g";
    case GLANE_UNIT_RPM:   return "rpm";
    case GLANE_UNIT_PCT:   return "%";
    case GLANE_UNIT_COUNT: return "count";
    default:               return "?";
    }
}

const char *glane_quality_name(uint8_t quality)
{
    switch (quality) {
    case GLANE_Q_OK:        return "ok";
    case GLANE_Q_STALE:     return "stale";
    case GLANE_Q_SATURATED: return "saturated";
    case GLANE_Q_FAULT:     return "fault";
    case GLANE_Q_UNCERTAIN: return "uncertain";
    default:                return "?";
    }
}

void glane_sample_init(glane_sample *s, uint32_t sensor_id, uint16_t unit)
{
    if (s == NULL) {
        return;
    }
    s->sensor_id   = sensor_id;
    s->seq         = 0;
    s->ts_ns       = 0;
    s->value_milli = 0;
    s->unit        = unit;
    s->quality     = GLANE_Q_OK;
    s->flags       = 0;
}

int glane_sample_emit(glane_sample *s, int32_t value_milli, int64_t ts_ns)
{
    if (s == NULL) {
        return -1;
    }
    if (ts_ns < s->ts_ns) {
        /* Jamais de retour en arrière : l'horodatage à la source est
         * monotone. Refuse la mesure plutôt que de corrompre la séquence. */
        return -1;
    }
    s->seq++;                  /* monotone, jamais réinitialisée           */
    s->value_milli = value_milli;
    s->ts_ns       = ts_ns;
    s->quality     = GLANE_Q_OK;
    return 0;
}

void glane_sample_flag(glane_sample *s, uint8_t bit, int set)
{
    if (s == NULL) {
        return;
    }
    if (set) {
        s->flags |= bit;
    } else {
        s->flags &= (uint8_t)~bit;
    }
}