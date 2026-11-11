/*
 * Glane — couche capteurs et publication de périphériques pour QNX 8.0
 * Copyright 2026 Baptiste Faisy. Licence Apache-2.0.
 *
 * glane_sample.h — ABI de la mesure (contrat GLA-02 / §5.1 du cahier des charges).
 *
 * Une mesure est un POD de 24 octets, sans pointeur, sans flottant :
 * rejouable bit à bit par un jumeau Python, copiable par memcpy, sérialisable
 * à l'octet. C'est LE contrat figé de tout le projet.
 */
#ifndef GLANE_SAMPLE_H
#define GLANE_SAMPLE_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ------------------------------------------------------------------ */
/* ABI de la mesure — 24 octets, alignement naturel, aucun pointeur     */
/* ------------------------------------------------------------------ */

typedef struct {
    uint32_t sensor_id;     /* identifiant issu de la description            */
    uint32_t seq;           /* numéro de séquence monotone, jamais réinitialisé */
    int64_t  ts_ns;         /* horodatage à la source (CLOCK_MONOTONIC)      */
    int32_t  value_milli;   /* valeur en milli-unités (échelle entière)      */
    uint16_t unit;          /* GLANE_UNIT_*                                  */
    uint8_t  quality;       /* GLANE_Q_*                                     */
    uint8_t  flags;         /* GLANE_F_*                                     */
} glane_sample;

/* Unités (échelle entière en milli-unités : GLANE_UNIT_C -> milli-°C). */
enum {
    GLANE_UNIT_C       = 1,  /* degrés Celsius               */
    GLANE_UNIT_K       = 2,  /* kelvins                     */
    GLANE_UNIT_V       = 3,  /* volts                       */
    GLANE_UNIT_MA      = 4,  /* milliampères                */
    GLANE_UNIT_DEG_S   = 5,  /* degrés par seconde          */
    GLANE_UNIT_G       = 6,  /* accélération (g)            */
    GLANE_UNIT_RPM     = 7,  /* tours par minute            */
    GLANE_UNIT_PCT     = 8,  /* pourcent                    */
    GLANE_UNIT_COUNT   = 9,  /* compteur / impulsion        */
};

/* Qualité : décrit la MESURE (jamais son origine — voir flags). */
enum {
    GLANE_Q_OK        = 0,
    GLANE_Q_STALE     = 1,  /* périmée, publiée au lieu d'être jetée        */
    GLANE_Q_SATURATED = 2,  /* hors plage de l'ADC / du capteur             */
    GLANE_Q_FAULT     = 3,  /* capteur muet / en panne                      */
    GLANE_Q_UNCERTAIN = 4,  /* plausibilité douteuse (capteur menteur, etc.)*/
};

/* Drapeaux : décrivent l'ORIGINE de la mesure. Bitmasks, cumulables. */
enum {
    GLANE_F_FIRST      = 1u << 0,  /* première mesure après attachement      */
    GLANE_F_CALIBRATED = 1u << 1,  /* calibration appliquée                 */
    GLANE_F_INJECTED   = 1u << 2,  /* valeur injectée (commande)            */
    GLANE_F_SIMULATED  = 1u << 3,  /* mesure synthétique (publisher virtuel) */
};

/* Taille cuite pour être vérifiée en test (doit valoir 24). */
#define GLANE_SAMPLE_SIZE 24

/* Nom lisible d'une unité ("C", "K", "V", "mA", "deg/s", "g", "rpm", "%",
 * "count") ou "?" si inconnu. Retourne une chaîne statique, jamais NULL.  */
const char *glane_unit_name(uint16_t unit);

/* Nom lisible d'une qualité ("ok", "stale", "saturated", "fault",
 * "uncertain") ou "?" si inconnu. Chaîne statique, jamais NULL.          */
const char *glane_quality_name(uint8_t quality);

/* Initialise une mesure : ids, seq=0, ts=0, value=0, quality=OK, flags=0. */
void glane_sample_init(glane_sample *s, uint32_t sensor_id, uint16_t unit);

/* Avance la séquence de façon monotone et pose la valeur + l'horodatage.
 * quality est remis à OK, flags conservé. Retourne 0, ou -1 si ts_ns est
 * antérieur au ts_ns déjà présent (jamais de retour en arrière).          */
int glane_sample_emit(glane_sample *s, int32_t value_milli, int64_t ts_ns);

/* Associe le drapeau `bit` à la mesure selon `set` (1 = poser, 0 = retirer). */
void glane_sample_flag(glane_sample *s, uint8_t bit, int set);

#ifdef __cplusplus
}
#endif

#endif /* GLANE_SAMPLE_H */