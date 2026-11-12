/*
 * Glane — Copyright 2026 Baptiste Faisy. Licence Apache-2.0.
 *
 * glane_publish.c — couche de publication (target/QNX).
 *
 * ÉTAT : échafaudage. La structure et l'interface sont prêtes ; les appels
 * PPS/resource-manager réels sont à écrire une fois le SDP QNX 8.0 installé
 * (le code QNX ne peut pas être compilé/vérifié sans qcc). En attendant, un
 * repli portable (une ligne compacte sur stdout) permet de compiler et de
 * tester la boucle du publisher sur Linux — et sur QNX avant implémentation.
 * Les blocs `#ifdef __QNX__` décrivent exactement ce qu'il faudra faire.
 */
#include "glane_publish.h"

#include <stdio.h>
#include <string.h>

#ifdef __QNX__
/* TODO(SDP) — inclure et implémenter avec le vrai SDP :
 *   #include <sys/resmgr.h>   (gestionnaire de ressources /dev/sensors)
 *   #include <pps.h>          (objets PPS, alternativement)
 * Ouvrir l'arborescence CONTRACT.md §6 :
 *   /dev/sensors/<name>/{value,quality,seq,ts_ns,unit,info}
 *   /dev/sensors/<name>/command/{period,calibrate,inject,selftest}
 * Référence : guide Device Publishers (dev_pub/rmps_objects.html) et l'exemple
 * officiel rpi-gpio (gitlab.com/qnx/projects) pour le pattern resmgr.
 */
#endif

int glane_publish_open(glane_publisher *p, const glane_desc *d)
{
    if (p == NULL || d == NULL) {
        return -1;
    }
    memset(p, 0, sizeof *p);
    p->desc = d;
    p->out_fd = 1;  /* repli stdout */
#ifdef __QNX__
    /* TODO(SDP): créer /dev/sensors/<name> + feuilles ; p->out_fd = fd PPS/resmgr.
     * Vérifier le nom avec glane_schema_validate_name(d->name) avant publication.
     * Retourner < 0 si la création échoue. */
#endif
    p->opened = 1;
    return 0;
}

int glane_publish_sample(glane_publisher *p, const glane_sample *s)
{
    if (p == NULL || !p->opened || s == NULL) {
        return -1;
    }
#ifdef __QNX__
    /* TODO(SDP): écrire les attributs puis notifier les abonnés. Exemple PPS :
     *   pps_output_printf(p->out_fd,
     *       "[value]%d\n[quality]%d\n[seq]%u\n[ts_ns]%lld\n[unit]%u\n",
     *       s->value_milli, s->quality, s->seq, (long long)s->ts_ns, s->unit);
     * Pour un resmgr : mettre à jour les entrées et lever ionotify.
     * En cas de succès, sauter le repli stdout ci-dessous. */
#endif
    /* Repli portable (hôte + QNX avant implémentation) : ligne compacte. */
    fprintf(stdout,
            "{\"s\":\"%s\",\"seq\":%u,\"ts_ns\":%lld,\"v_m\":%d,\"q\":%u,\"f\":%u}\n",
            p->desc->name, (unsigned)s->seq, (long long)s->ts_ns,
            (int)s->value_milli, (unsigned)s->quality, (unsigned)s->flags);
    p->published++;
    return 0;
}

void glane_publish_close(glane_publisher *p)
{
    if (p == NULL) {
        return;
    }
#ifdef __QNX__
    /* TODO(SDP): détruire les objets publiés, fermer p->out_fd. */
#endif
    p->opened = 0;
}