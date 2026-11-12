/*
 * Glane — Copyright 2026 Baptiste Faisy. Licence Apache-2.0.
 *
 * virtuel.c — générateur de mesures synthétiques déterministe (GLA-03/04).
 * Pur : ts_ns injecté, aucune allocation, aucun état global. Le bruit est une
 * fonction de hachage pure de (seed, step) — pas d'état mutable, donc deux
 * rejeux donnent exactement la même suite (rejeu déterministe octet pour octet).
 */
#include "glane/glane_virtuel.h"

#include <limits.h>
#include <stddef.h>

/* Table sinusoïdale Q15, 64 points : round(sin(2*pi*k/64) * 32767). */
static const int16_t sine_q15[64] = {
    0, 3212, 6393, 9512, 12539, 15446, 18204, 20787,
    23170, 25329, 27245, 28898, 30273, 31356, 32137, 32609,
    32767, 32609, 32137, 31356, 30273, 28898, 27245, 25329,
    23170, 20787, 18204, 15446, 12539, 9512, 6393, 3212,
    0, -3212, -6393, -9512, -12539, -15446, -18204, -20787,
    -23170, -25329, -27245, -28898, -30273, -31356, -32137, -32609,
    -32767, -32609, -32137, -31356, -30273, -28898, -27245, -25329,
    -23170, -20787, -18204, -15446, -12539, -9512, -6393, -3212,
};

/* Hachage entier pur de (seed, step) -> 15 bits, déterministe et sans état. */
static uint32_t hash_noise(uint32_t seed, uint32_t step)
{
    uint32_t h = seed ^ (step * 2654435761u);
    h ^= h >> 16;
    h *= 0x7feb352du;
    h ^= h >> 15;
    h *= 0x846ca68bu;
    h ^= h >> 16;
    return h & 0x7FFFu;  /* 0..32767 */
}

void glane_virtuel_init(glane_virtuel *v, const glane_virtuel_cfg *cfg)
{
    if (v == NULL || cfg == NULL) {
        return;
    }
    v->cfg  = *cfg;
    v->step = 0;
    v->seq  = 0;
    if (v->cfg.period_steps < 1) {
        v->cfg.period_steps = 1;
    }
}

int32_t glane_virtuel_wave(const glane_virtuel *v, uint32_t step)
{
    if (v == NULL) {
        return 0;
    }
    int32_t amp = v->cfg.amp_milli;
    int32_t per = v->cfg.period_steps;
    uint32_t phase = (per > 0) ? (step % (uint32_t)per) : 0;
    switch (v->cfg.wf) {
    case GLANE_WF_CONST:
        return 0;
    case GLANE_WF_SINE: {
        uint32_t idx = (uint32_t)(((uint64_t)step * 64u) / (uint32_t)per) % 64u;
        return (int32_t)(((int64_t)amp * sine_q15[idx]) / 32767);
    }
    case GLANE_WF_RAMP:
        /* dent de scie : -amp à +amp sur la période */
        return (int32_t)(((int64_t)amp *
                          (2 * (int64_t)phase - per)) / per);
    case GLANE_WF_TRIANGLE: {
        int32_t half = per / 2;
        if (half <= 0) {
            return 0;
        }
        if (phase < (uint32_t)half) {
            return (int32_t)(-amp + ((int64_t)2 * amp * phase) / half);
        }
        int32_t rest = per - half;
        return (int32_t)(amp - ((int64_t)2 * amp * (phase - (uint32_t)half)) /
                         (rest > 0 ? rest : 1));
    }
    case GLANE_WF_NOISE: {
        uint32_t r = hash_noise(v->cfg.seed, step);  /* 0..32767 */
        return (int32_t)(((int64_t)r * amp) / 16384) - amp;  /* -amp..+amp */
    }
    default:
        return 0;
    }
}

int glane_virtuel_next(glane_virtuel *v, int64_t ts_ns, glane_sample *out)
{
    if (v == NULL || out == NULL) {
        return -1;
    }
    int64_t raw = (int64_t)v->cfg.base_milli +
                  glane_virtuel_wave(v, v->step) +
                  (int64_t)v->cfg.drift_per_step * v->step;
    uint8_t q = GLANE_Q_OK;
    if (v->cfg.sat_min_milli < v->cfg.sat_max_milli) {
        if (raw < v->cfg.sat_min_milli) {
            raw = v->cfg.sat_min_milli;
            q = GLANE_Q_SATURATED;
        } else if (raw > v->cfg.sat_max_milli) {
            raw = v->cfg.sat_max_milli;
            q = GLANE_Q_SATURATED;
        }
    }
    if (raw > (int64_t)INT32_MAX) {
        raw = INT32_MAX;
        q = GLANE_Q_SATURATED;
    } else if (raw < (int64_t)INT32_MIN) {
        raw = INT32_MIN;
        q = GLANE_Q_SATURATED;
    }
    out->sensor_id   = v->cfg.sensor_id;
    out->unit        = v->cfg.unit;
    out->ts_ns       = ts_ns;
    out->value_milli = (int32_t)raw;
    out->quality     = q;
    out->seq         = ++v->seq;
    out->flags       = GLANE_F_SIMULATED |
                       (v->seq == 1 ? GLANE_F_FIRST : 0);
    v->step++;
    return 0;
}