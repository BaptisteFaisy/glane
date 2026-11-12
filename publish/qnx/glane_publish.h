/*
 * Glane — Copyright 2026 Baptiste Faisy. Licence Apache-2.0.
 *
 * glane_publish.h — interface de la couche de publication (target/QNX).
 *
 * C'est le SEUL point de contact avec le mécanisme de publication du système.
 * Sur QNX (__QNX__), l'implémentation crée les objets dans l'arborescence
 * /dev/sensors/<name> (PPS / resource manager) et notifie les abonnés.
 * Ailleurs (build hôte), c'est un repli stdout qui permet de compiler et de
 * tester la boucle du publisher sans SDP. Le socle core/ reste pur : cette
 * couche est la seule à connaître l'OS.
 */
#ifndef GLANE_PUBLISH_H
#define GLANE_PUBLISH_H

#include "glane/glane_desc.h"
#include "glane/glane_sample.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    const glane_desc *desc;   /* description du capteur publié            */
    int opened;               /* 1 si ouvert avec succès                  */
    int out_fd;               /* QNX : descripteur PPS/resmgr ; hôte : 1  */
    unsigned long published;  /* compteur de mesures publiées             */
} glane_publisher;

/* Ouvre la publication pour un capteur : crée l'objet /dev/sensors/<name>
 * et ses feuilles (value, quality, seq, ts_ns, unit, info) + command.
 * Retourne 0 si OK, sinon un code négatif.                              */
int glane_publish_open(glane_publisher *p, const glane_desc *d);

/* Publie une mesure : met à jour value/quality/seq/ts_ns et notifie les
 * abonnés. Retourne 0 si OK.                                            */
int glane_publish_sample(glane_publisher *p, const glane_sample *s);

/* Ferme et détruit les objets publiés. */
void glane_publish_close(glane_publisher *p);

#ifdef __cplusplus
}
#endif

#endif /* GLANE_PUBLISH_H */