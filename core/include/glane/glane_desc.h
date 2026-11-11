/*
 * Glane — Copyright 2026 Baptiste Faisy. Licence Apache-2.0.
 *
 * glane_desc.h — description déclarative d'un capteur (GLA-02, §5.2).
 *
 * Une description est un schéma JSON (fichiers `sensors.d/<nom>.json`).
 * `glane_desc_parse` analyse une chaîne JSON (aucune allocation : tout tient
 * dans les tampons fournis par l'appelant) et remplit un `glane_desc` + un
 * `glane_desc_error` précis (champ, ligne, cause) en cas d'erreur.
 */
#ifndef GLANE_DESC_H
#define GLANE_DESC_H

#include <stdint.h>
#include <stddef.h>

#include "glane_sample.h"
#include "glane_calib.h"
#include "glane_bus.h"

#ifdef __cplusplus
extern "C" {
#endif

#define GLANE_NAME_MAX 64   /* nom de capteur / driver / device, NUL inclus */
#define GLANE_DESC_ERR_MAX 160  /* message "cause" NUL inclus               */

typedef struct {
    uint32_t id;                              /* champ `id` (requis, > 0)   */
    char     name[GLANE_NAME_MAX];            /* champ `name` (requis)      */
    char     driver[GLANE_NAME_MAX];          /* champ `driver` (requis)    */
    int32_t  bus_kind;                        /* GLANE_BUS_* (requis)       */
    char     bus_dev[GLANE_NAME_MAX];         /* champ `bus.dev`            */
    uint32_t bus_addr;                        /* champ `bus.addr`, entier   */
    uint32_t period_ms;                       /* champ `period_ms` (> 0)    */
    uint32_t deadline_ms;                     /* champ `deadline_ms` (>= 0) */
    uint32_t stale_after_ms;                  /* champ `stale_after_ms` (>0)*/
    uint16_t unit;                            /* champ `unit` -> GLANE_UNIT_* */
    int32_t  range_min_milli;                 /* champ `range[0]` (milli)   */
    int32_t  range_max_milli;                 /* champ `range[1]` (milli)   */
    int32_t  warn_milli;                      /* champ `thresholds.warn`    */
    int32_t  crit_milli;                      /* champ `thresholds.crit`    */
    glane_calib calib;                        /* champ `calib` (optionnel)  */
} glane_desc;

/* Codes d'erreur de glane_desc_parse (le détail humain est dans `cause`). */
enum {
    GLANE_DESC_OK          = 0,  /* analyse réussie                        */
    GLANE_DESC_EJSON       = 1,  /* JSON mal formé (syntaxe)               */
    GLANE_DESC_EFIELD      = 2,  /* champ manquant                         */
    GLANE_DESC_ETYPE       = 3,  /* type inattendu (nombre vs chaîne...)   */
    GLANE_DESC_ERANGE      = 4,  /* valeur hors plage / incohérente        */
    GLANE_DESC_EUNIT       = 5,  /* unité inconnue                         */
    GLANE_DESC_EBUS        = 6,  /* bus.kind inconnu                       */
    GLANE_DESC_ECALIB      = 7,  /* calibration invalide                   */
};

typedef struct {
    int  code;                             /* GLANE_DESC_* (0 = OK)         */
    char field[GLANE_NAME_MAX];            /* dernier champ visité (ou "")  */
    int  line;                             /* numéro de ligne (1-based, 0)  */
    char cause[GLANE_DESC_ERR_MAX];        /* message humain précis         */
} glane_desc_error;

/* Parse un JSON de description. `json` peut ne pas être NUL-terminé : sa
 * longueur est `len`. Remplit `out` seulement en cas de succès (code 0).
 * Retourne le code d'erreur (0 = OK) ; `*err` reçoit le détail dans tous
 * les cas. Jamais d'allocation, jamais de lecture hors tampon.            */
int glane_desc_parse(const char *json, size_t len, glane_desc *out,
                     glane_desc_error *err);

/* Vérifie la cohérence d'une description déjà remplie (plages, seuils,
 * ordre range_min <= range_max, périodes, degré de calibration). Retourne
 * un code GLANE_DESC_* (0 = valide).                                       */
int glane_desc_validate(const glane_desc *d, glane_desc_error *err);

/* Message court par code d'erreur (statique, jamais NULL). */
const char *glane_desc_errstr(int code);

#ifdef __cplusplus
}
#endif

#endif /* GLANE_DESC_H */