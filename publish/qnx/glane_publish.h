/*
 * Glane — Copyright 2026 Baptiste Faisy. Licence Apache-2.0.
 *
 * glane_publish.h — interface de la couche de publication (target/QNX).
 *
 * La publication crée, dans l'espace de noms de la cible, un objet par capteur
 * avec ses feuilles : value, quality, seq, ts_ns, unit, info (+ command/).
 * C'est une vraie publication (des fichiers réels dans le namespace, lisibles
 * par `cat`, par un abonné ou un gestionnaire de ressources) — pas un simple
 * journal sur stdout. Le socle core/ reste pur : cette couche est la seule à
 * écrire dans le système de fichiers.
 */
#ifndef GLANE_PUBLISH_H
#define GLANE_PUBLISH_H

#include "glane/glane_desc.h"
#include "glane/glane_sample.h"

#ifdef __cplusplus
extern "C" {
#endif

#define GLANE_PUBLISH_LEAVES 6   /* value, quality, seq, ts_ns, unit, info  */

/* Taille du cache « dernier contenu écrit » par feuille. Les feuilles
 * scalaires sont courtes (valeur, qualité, séquence, horodatage, unité) ;
 * 64 octets suffisent largement. */
#define LEAF_CACHE_MAX 64

typedef struct {
    const glane_desc *desc;   /* description du capteur publié            */
    int opened;               /* 1 si ouvert avec succès                  */
    int use_files;            /* 1 si publication fichier active          */
    int flat;                 /* 1 = fichiers à plat (pas d'arborescence)  */
    int fds[GLANE_PUBLISH_LEAVES];   /* descripteurs des feuilles         */
    char root[160];           /* racine publiée (ex. /sensors)           */
    int out_fd;               /* repli stdout si use_files == 0          */
    unsigned long published;  /* compteur de mesures publiées             */
    long info_bytes;          /* taille du journal `info` (borne)         */
    int reuse_fds;            /* 1 = réécrire en place (pwrite+ftruncate)  */
    size_t leaf_len[GLANE_PUBLISH_LEAVES]; /* taille de chaque feuille       */
    char leaf_cache[GLANE_PUBLISH_LEAVES][LEAF_CACHE_MAX]; /* dernier contenu */
} glane_publisher;

/* Ouvre la publication pour un capteur : crée <root>/<name>/{value,quality,
 * seq,ts_ns,unit,info} (+ <name>/command/). La racine est lue de
 * l'environnement GLANE_SENSOR_DIR, sinon "/sensors". Si l'écriture de fichiers
 * est impossible, bascule sur stdout (repli portable). Retourne 0 si OK.   */
int glane_publish_open(glane_publisher *p, const glane_desc *d);

/* Publie une mesure : écrit la valeur et ses métadonnées dans les feuilles.
 * Retourne 0 si OK.                                                       */
int glane_publish_sample(glane_publisher *p, const glane_sample *s);

/* Ferme les feuilles et libère. */
void glane_publish_close(glane_publisher *p);

#ifdef __cplusplus
}
#endif

#endif /* GLANE_PUBLISH_H */