/*
 * Glane — Copyright 2026 Baptiste Faisy. Licence Apache-2.0.
 *
 * glane_deadline.h — péremption et panne (GLA-06, GLA-07).
 *
 * Une mesure est publiée STALE au lieu d'être jetée quand elle dépasse
 * `stale_after` ; un capteur qui ne répond plus passe FAULT après N
 * périodes manquantes. Aucune suppression silencieuse.
 */
#ifndef GLANE_DEADLINE_H
#define GLANE_DEADLINE_H

#include <stdint.h>

#include "glane_sample.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    int64_t  deadline_ns;     /* `deadline_ms` convertie en ns             */
    int64_t  stale_after_ns;  /* `stale_after_ms` convertie en ns          */
    uint32_t fault_after;     /* périodes manquantes avant FAULT (> 0)     */
    uint32_t missing;         /* compteur de périodes manquantes consécut. */
} glane_deadline;

/* Initialise le suivi. miss_before_fault = nombre de périodes manquantes
 * consécutives avant de déclarer FAULT (>= 1).                            */
void glane_deadline_init(glane_deadline *d, uint32_t deadline_ms,
                         uint32_t stale_after_ms, uint32_t miss_before_fault);

/* Qualité à appliquer à une mesure d'âge `age_ns` (now - ts_ns) :
 * OK si age_ns <= stale_after_ns, STALE sinon. Ne modifie jamais la
 * mesure : l'appelant applique le résultat.                               */
uint8_t glane_deadline_quality(const glane_deadline *d, int64_t age_ns);

/* Signale une période sans mesure : incrémente `missing` et rend la
 * qualité FAULT dès que fault_after est atteint, sinon STALE.              */
uint8_t glane_deadline_missed(glane_deadline *d);

/* Compte une mesure fraîche : remet `missing` à zéro (capteur vivant).    */
void glane_deadline_alive(glane_deadline *d);

#ifdef __cplusplus
}
#endif

#endif /* GLANE_DEADLINE_H */