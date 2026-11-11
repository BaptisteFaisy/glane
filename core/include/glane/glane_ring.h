/*
 * Glane — Copyright 2026 Baptiste Faisy. Licence Apache-2.0.
 *
 * glane_ring.h — tampon circulaire de mesures (GLA-03, GLA-16).
 *
 * Pas d'allocation : le tampon de stockage est fourni par l'appelant.
 * Sert côté abonné à détecter les trous de `seq` (pertes) et à rejouer
 * dans l'ordre.
 */
#ifndef GLANE_RING_H
#define GLANE_RING_H

#include <stddef.h>

#include "glane_sample.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    glane_sample *buf;   /* stockage fourni par l'appelant (cap éléments)  */
    size_t cap;          /* capacité (>= 1)                                  */
    size_t head;         /* indice de la prochaine écriture                */
    size_t count;        /* nombre d'éléments présents (<= cap)            */
} glane_ring;

/* Initialise un anneau sur un tampon de `cap` éléments. */
void glane_ring_init(glane_ring *r, glane_sample *buf, size_t cap);

/* Ajoute une mesure. Si l'anneau est plein, écrase la plus ancienne et
 * retourne 1 ; sinon retourne 0.                                          */
int glane_ring_push(glane_ring *r, const glane_sample *s);

/* Lit le i-ème élément, i=0 étant le plus ancien encore présent.
 * Retourne 0 si i < count, sinon -1 (bordures).                           */
int glane_ring_peek(const glane_ring *r, size_t i, glane_sample *out);

/* Nombre d'éléments présents. */
size_t glane_ring_count(const glane_ring *r);

/* Détecte un trou de séquence entre l'élément le plus ancien et le plus
 * récent de l'anneau (les valeurs de `seq` doivent être consécutives).
 * Si le premier trou est en position p (0 = entre [0] et [1]), remplit
 * *first_gap et retourne p. Retourne -1 si aucune perte détectée.
 * Le retour à zéro de `seq` (wrap) n'est PAS compté comme un trou.        */
int glane_ring_find_gap(const glane_ring *r, size_t *first_gap);

#ifdef __cplusplus
}
#endif

#endif /* GLANE_RING_H */