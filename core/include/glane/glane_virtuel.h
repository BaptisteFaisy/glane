/*
 * Glane — Copyright 2026 Baptiste Faisy. Licence Apache-2.0.
 *
 * glane_virtuel.h — générateur de mesures synthétiques déterministe (GLA-03,
 * GLA-04, idée B2/B3). Pur : aucune horloge (ts_ns injecté), aucune allocation,
 * aucun état global. Deux exécutions avec la même configuration et la même
 * graine produisent exactement la même suite de mesures — c'est la base du
 * rejeu déterministe et du publisher virtuel sous QEMU.
 */
#ifndef GLANE_VIRTUEL_H
#define GLANE_VIRTUEL_H

#include <stdint.h>

#include "glane_sample.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Formes d'onde disponibles (toutes en arithmétique entière). */
typedef enum {
    GLANE_WF_CONST    = 0,  /* valeur constante + dérive                  */
    GLANE_WF_SINE     = 1,  /* sinusoïde (table Q15, 64 points)           */
    GLANE_WF_RAMP     = 2,  /* dent de scie -amp..+amp                    */
    GLANE_WF_TRIANGLE = 3,  /* triangle -amp..+amp                        */
    GLANE_WF_NOISE    = 4,  /* bruit pseudo-aléatoire déterministe (LCG)  */
} glane_waveform;

typedef struct {
    uint32_t sensor_id;
    uint16_t unit;
    int32_t  wf;              /* glane_waveform                          */
    int32_t  base_milli;      /* offset / valeur moyenne (milli)         */
    int32_t  amp_milli;       /* amplitude (sine/ramp/tri/noise, milli)  */
    int32_t  period_steps;    /* pas par période (>= 1)                  */
    int32_t  drift_per_step;  /* dérive ajoutée à chaque pas (milli)     */
    uint32_t seed;            /* graine du LCG (bruit reproductible)     */
    int32_t  sat_min_milli;   /* borne basse de saturation               */
    int32_t  sat_max_milli;   /* borne haute de saturation               */
} glane_virtuel_cfg;

typedef struct {
    glane_virtuel_cfg cfg;
    uint32_t step;   /* pas courant (avance à chaque next)               */
    uint32_t seq;    /* séquence monotone                                */
} glane_virtuel;

/* Initialise le générateur (copie la configuration, remet step/seq à 0). */
void glane_virtuel_init(glane_virtuel *v, const glane_virtuel_cfg *cfg);

/* Valeur brute de la forme d'onde au pas `step`, avant saturation/dérive. */
int32_t glane_virtuel_wave(const glane_virtuel *v, uint32_t step);

/* Génère la mesure suivante à l'horodatage injecté `ts_ns`. Applique la
 * dérive puis la saturation (qualité SATURATED si bornes dépassées, OK
 * sinon). Avance step et seq. Déterministe. Retourne 0, ou -1 si out NULL. */
int glane_virtuel_next(glane_virtuel *v, int64_t ts_ns, glane_sample *out);

#ifdef __cplusplus
}
#endif

#endif /* GLANE_VIRTUEL_H */