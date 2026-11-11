/*
 * Glane — Copyright 2026 Baptiste Faisy. Licence Apache-2.0.
 *
 * desc_validate.c — validation sémantique d'une description (GLA-02).
 * Séparé de desc.c (analyse) pour rester sous les bornes de taille et
 * séparer les responsabilités. Réutilise glane_schema_validate_name et
 * glane_unit_name : zéro duplication.
 */
#include "glane/glane_desc.h"
#include "glane/glane_jsonscan.h"  /* json_fail */
#include "glane/glane_schema.h"

#include <stddef.h>

/* Borne des coefficients de calibration (CONTRACT.md §2.1). */
#define CALIB_COEF_BOUND 1048575LL

int glane_desc_validate(const glane_desc *d, glane_desc_error *err)
{
    if (d == NULL) {
        return GLANE_DESC_EFIELD;
    }
    if (d->id == 0)
        return json_fail(err, GLANE_DESC_ERANGE, "id", 0, "id doit être > 0");
    if (glane_schema_validate_name(d->name, SIZE_MAX) != GLANE_SCHEMA_OK)
        return json_fail(err, GLANE_DESC_EFIELD, "name", 0,
                         "nom de capteur invalide");
    if (glane_schema_validate_name(d->driver, SIZE_MAX) != GLANE_SCHEMA_OK)
        return json_fail(err, GLANE_DESC_EFIELD, "driver", 0,
                         "nom de driver invalide");
    if (d->bus_kind < GLANE_BUS_I2C || d->bus_kind > GLANE_BUS_VIRTUEL)
        return json_fail(err, GLANE_DESC_EBUS, "bus.kind", 0,
                         "bus.kind hors plage");
    if (d->period_ms == 0)
        return json_fail(err, GLANE_DESC_ERANGE, "period_ms", 0,
                         "period_ms doit être > 0");
    if (d->stale_after_ms == 0)
        return json_fail(err, GLANE_DESC_ERANGE, "stale_after_ms", 0,
                         "stale_after_ms doit être > 0");
    if (d->range_min_milli > d->range_max_milli)
        return json_fail(err, GLANE_DESC_ERANGE, "range", 0,
                         "range[0] doit être <= range[1]");
    if (glane_unit_name(d->unit)[0] == '?')
        return json_fail(err, GLANE_DESC_EUNIT, "unit", 0, "unité inconnue");
    if (d->calib.deg > GLANE_CALIB_MAX_DEG)
        return json_fail(err, GLANE_DESC_ECALIB, "calib.deg", 0,
                         "degré de calibration > 3");
    for (int i = 0; i <= d->calib.deg && i <= GLANE_CALIB_MAX_DEG; i++) {
        if (d->calib.coef[i] > CALIB_COEF_BOUND ||
            d->calib.coef[i] < -CALIB_COEF_BOUND)
            return json_fail(err, GLANE_DESC_ECALIB, "calib.coef", 0,
                             "coefficient hors borne (CONTRACT §2.1)");
    }
    if (err != NULL) {
        err->code = GLANE_DESC_OK;
        err->field[0] = '\0';
        err->line = 0;
        err->cause[0] = '\0';
    }
    return GLANE_DESC_OK;
}

const char *glane_desc_errstr(int code)
{
    switch (code) {
    case GLANE_DESC_OK:     return "ok";
    case GLANE_DESC_EJSON:  return "JSON mal formé";
    case GLANE_DESC_EFIELD: return "champ manquant ou invalide";
    case GLANE_DESC_ETYPE:  return "type inattendu";
    case GLANE_DESC_ERANGE: return "valeur hors plage";
    case GLANE_DESC_EUNIT:  return "unité inconnue";
    case GLANE_DESC_EBUS:   return "bus.kind inconnu";
    case GLANE_DESC_ECALIB: return "calibration invalide";
    default:                return "erreur inconnue";
    }
}