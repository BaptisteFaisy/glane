/*
 * Glane — Copyright 2026 Baptiste Faisy. Licence Apache-2.0.
 *
 * glane_bus.h — abstraction de bus (GLA-03 / §4 du cahier des charges).
 *
 * core/ ne connaît ni horloge, ni fichier, ni socket, ni allocation, ni état
 * global. Toute lecture d'un capteur passe par un pointeur de fonction
 * injecté par l'appelant : c'est le seul point de contact avec le monde.
 */
#ifndef GLANE_BUS_H
#define GLANE_BUS_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Type de bus (aussi le champ `bus.kind` des descriptions JSON en 5.2). */
enum {
    GLANE_BUS_I2C     = 1,
    GLANE_BUS_SPI     = 2,
    GLANE_BUS_GPIO    = 3,
    GLANE_BUS_FILE    = 4,  /* thermique sysfs-like, pseudo-fichier        */
    GLANE_BUS_VIRTUEL = 5,  /* mesures synthétiques, aucun matériel        */
};

/* Un lecteur de bus : remplit *value_milli (brute, milli-unités) et
 * *ts_ns (horodatage à la source, CLOCK_MONOTONIC). `ctx` est opaque.
 * Retourne 0 si OK, sinon un code d'erreur non nul (cf. implémentation
 * de bus/). core/ se contente de propager la valeur de retour.            */
typedef int (*glane_bus_read_fn)(void *ctx, int32_t *value_milli, int64_t *ts_ns);

#ifdef __cplusplus
}
#endif

#endif /* GLANE_BUS_H */