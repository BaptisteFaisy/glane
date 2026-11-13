/*
 * Glane — Copyright 2026 Baptiste Faisy. Licence Apache-2.0.
 *
 * glane_publish.c — couche de publication (target/QNX).
 *
 * Publication RÉELLE : glaned crée dans l'espace de noms de la cible un objet
 * par capteur et y écrit les feuilles value / quality / seq / ts_ns / unit /
 * info. Un abonné (cat, un client PPS, un gestionnaire de ressources) lit donc
 * de vraies données publiées, pas une trace sur stdout.
 *
 * La racine est configurable : GLANE_SENSOR_DIR (défaut "/sensors"). Si le
 * système de fichiers n'est pas inscriptible, repli sur stdout — l'objet
 * publié reste alors visible dans le journal.
 */
#define _QNX_SOURCE 1

#include "glane_publish.h"

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define PUBLISH_ROOT_DEFAULT "/sensors"

/* Journal `info` : on ne garde que les DERNIERES lignes. Sans borne, un
 * publisher qui tourne en continu fait croitre le tmpfs de l'invite sans fin
 * (la capture de la vitrine montrait deja 1 200 lignes et counting). */
#define INFO_MAX_BYTES 8192
#define INFO_KEEP_LINES 16

static const char *const leaves[GLANE_PUBLISH_LEAVES] = {
    "value", "quality", "seq", "ts_ns", "unit", "info",
};

/* mkdir -p : crée le chemin et tous ses parents. Retourne 0 si OK. */
static int mkdir_p(const char *path)
{
    char tmp[256];
    size_t len = strlen(path);
    if (len == 0 || len >= sizeof tmp) {
        return -1;
    }
    memcpy(tmp, path, len + 1);
    for (size_t i = 1; i < len; i++) {
        if (tmp[i] != '/') {
            continue;
        }
        tmp[i] = '\0';
        if (mkdir(tmp, 0755) != 0 && access(tmp, F_OK) != 0) {
            return -1;
        }
        tmp[i] = '/';
    }
    if (mkdir(tmp, 0755) != 0 && access(tmp, F_OK) != 0) {
        return -1;
    }
    return 0;
}

/* Écrit une chaîne dans un descripteur (ignore les erreurs partielles). */
static void put(int fd, const char *text)
{
    if (fd < 0 || text == NULL) {
        return;
    }
    ssize_t n = write(fd, text, strlen(text));
    (void)n;
}

/* Racines de publication candidates, dans l'ordre. La première inscriptible
 * gagne. L'environnement (GLANE_SENSOR_DIR) passe devant. Un vrai
 * déploiement QNX montera /dev/sensors (resmgr/PPS) ; en environnement
 * minimal on tombe sur un tmpfs de l'invité. */
static const char *const root_candidates[] = {
    "/dev/shmem/sensors",
    "/tmp/sensors",
    "/sensors",
    "/var/sensors",
    NULL,
};

/* Chemin d'une feuille : arborescence <root>/<nom>/<feuille>, ou à plat
 * <root>/<nom>.<feuille> quand l'arborescence n'est pas creatable (certains
 * tmpfs QNX n'implémentent pas mkdir). */
static void leaf_path(const glane_publisher *p, int i, char *buf, size_t bufsz)
{
    if (p->flat) {
        snprintf(buf, bufsz, "%s/%s.%s", p->root, p->desc->name, leaves[i]);
    } else {
        snprintf(buf, bufsz, "%s/%s/%s", p->root, p->desc->name, leaves[i]);
    }
}

/* Réécrit une feuille en place : le descripteur reste OUVERT.
 *
 * Deux contraintes :
 *  - CONTRACT.md §6 : une feuille porte l'état COURANT du capteur, pas un
 *    historique. Rouvrir le fichier à chaque écriture (close + open +
 *    write) marchait, mais coûte 5 open()+close() par mesure sur le tmpfs
 *    de l'invité : mesuré sur QNX 8.0, la latence de publication est
 *    passée de p50 = 0,72 ms à p50 = 3,65 ms (×5) après ce correctif.
 *  - donc on réécrit EN PLACE, en une écriture positionnée (pwrite) et une
 *    troncature (ftruncate) — l'ordre dépend de si la valeur rétrécit ou
 *    grandit : on n'écrit JAMAIS dans un objet plus petit que la donnée.
 *
 * ftruncate/pwrite ne sont pas garantis par tous les systèmes de fichiers :
 * si l'appel échoue, on retombe une fois pour toutes sur la réouverture
 * (chemin correct, plus lent) et on le signale.
 */
static void put_leaf(glane_publisher *p, int i, const char *text)
{
    size_t len;
    int ok = 0;
    if (p->fds[i] < 0 || text == NULL) {
        return;
    }
    len = strlen(text);
    if (p->reuse_fds) {
        /* Feuilles constantes (qualité, unité) : rien à réécrire si le
         * contenu n'a pas bougé — 2 écritures syscall en moins par mesure. */
        if (len <= LEAF_CACHE_MAX &&
            p->leaf_len[i] == len &&
            memcmp(p->leaf_cache[i], text, len) == 0) {
            return;
        }
        if (len >= p->leaf_len[i]) {
            /* la valeur grandit : fixer la taille AVANT d'écrire */
            ok = (ftruncate(p->fds[i], (off_t)len) == 0 &&
                  pwrite(p->fds[i], text, len, 0) == (ssize_t)len);
        } else {
            /* la valeur rétrécit : écrire d'abord, tronquer ensuite */
            ok = (pwrite(p->fds[i], text, len, 0) == (ssize_t)len &&
                  ftruncate(p->fds[i], (off_t)len) == 0);
        }
        if (ok) {
            p->leaf_len[i] = len;
            if (len <= LEAF_CACHE_MAX) {
                memcpy(p->leaf_cache[i], text, len);
            }
            return;
        }
        /* ftruncate/pwrite indisponibles ici : repli en réouverture. */
        p->reuse_fds = 0;
        fprintf(stderr, "glane-publish: reecriture en place impossible "
                        "(feuille %s) — repli en reouverture\n", leaves[i]);
    }
    {
        char path[512];
        leaf_path(p, i, path, sizeof path);
        close(p->fds[i]);
        p->fds[i] = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (p->fds[i] < 0) {
            return;
        }
        put(p->fds[i], text);
        p->leaf_len[i] = len;
    }
}

/* Compacte le journal `info` : on ne garde que les dernieres lignes. Relit
 * le fichier, recupere les N dernieres lignes, le reecrit a zero. */
static void compact_info(glane_publisher *p)
{
    static char buf[INFO_MAX_BYTES + 512];
    char path[512];
    long n;
    long start = 0;
    int kept = 0;
    leaf_path(p, 5, path, sizeof path);
    int fd = open(path, O_RDONLY);
    if (fd < 0) {
        return;
    }
    n = read(fd, buf, sizeof buf - 1);
    close(fd);
    if (n <= 0) {
        return;
    }
    buf[n] = '\0';
    /* Balaye a rebours pour trouver le debut de la INFO_KEEP_LINES-ieme
     * derniere ligne. */
    for (long i = n - 1; i >= 0; i--) {
        if (buf[i] == '\n' && i + 1 < n) {
            kept++;
            if (kept >= INFO_KEEP_LINES) {
                start = i + 1;
                break;
            }
        }
    }
    if (p->fds[5] >= 0) {
        close(p->fds[5]);
    }
    p->fds[5] = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (p->fds[5] < 0) {
        return;
    }
    put(p->fds[5], buf + start);
    p->info_bytes = n - start;
}

/* Tente d'ouvrir les feuilles dans une racine donnée. Retourne 1 si la
 * publication fichier est en place, 0 sinon (racine suivante à essayer). */
static int try_root(glane_publisher *p, const char *root)
{
    char dir[512];
    snprintf(dir, sizeof dir, "%.159s/%.63s", root, p->desc->name);
    /* Si l'arborescence n'est pas creatable (mkdir non implémenté sur
     * certains tmpfs QNX), on publie à plat dans la même racine. */
    p->flat = (mkdir_p(dir) != 0);
    snprintf(p->root, sizeof p->root, "%.159s", root);
    if (!p->flat) {
        snprintf(dir, sizeof dir, "%.159s/%.63s/command",
                 p->root, p->desc->name);
        (void)mkdir_p(dir);
    }
    for (int i = 0; i < GLANE_PUBLISH_LEAVES; i++) {
        char path[512];
        leaf_path(p, i, path, sizeof path);
        p->fds[i] = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (p->fds[i] < 0) {
            return 0;
        }
    }
    fprintf(stderr, "glane-publish: racine publiee = %s (%s)\n",
            p->root, p->flat ? "plat" : "arborescence");
    p->use_files = 1;
    return 1;
}

int glane_publish_open(glane_publisher *p, const glane_desc *d)
{
    if (p == NULL || d == NULL) {
        return -1;
    }
    memset(p, 0, sizeof *p);
    p->desc = d;
    p->out_fd = 1;
    /* Réécriture en place des feuilles (pwrite + ftruncate) : évite 5
     * open()+close() par mesure, qui coûtaient ×5 en latence sur QNX. */
    p->reuse_fds = 1;
    for (int i = 0; i < GLANE_PUBLISH_LEAVES; i++) {
        p->fds[i] = -1;
    }
    /* Liste des racines à essayer : env d'abord, puis les candidats. */
    char roots[6][160];
    int nroots = 0;
    const char *env = getenv("GLANE_SENSOR_DIR");
    if (env != NULL && env[0] != '\0') {
        snprintf(roots[nroots], sizeof roots[0], "%.159s", env);
        nroots++;
    }
    for (int i = 0; root_candidates[i] != NULL && nroots < 6; i++) {
        snprintf(roots[nroots], sizeof roots[0], "%s", root_candidates[i]);
        nroots++;
    }
    for (int r = 0; r < nroots; r++) {
        if (try_root(p, roots[r])) {
            p->opened = 1;
            return 0;
        }
        glane_publish_close(p);
    }
    fprintf(stderr, "glane-publish: aucune racine inscriptible, repli stdout\n");
    p->opened = 1;
    return 0;
}

int glane_publish_sample(glane_publisher *p, const glane_sample *s)
{
    if (p == NULL || !p->opened || s == NULL) {
        return -1;
    }
    char buf[256];
    if (!p->use_files) {
        fprintf(stdout,
                "{\"s\":\"%s\",\"seq\":%u,\"ts_ns\":%lld,\"v_m\":%d,\"u\":\"%s\","
                "\"q\":\"%s\",\"f\":%u}\n",
                p->desc->name, (unsigned)s->seq, (long long)s->ts_ns,
                (int)s->value_milli, glane_unit_name(s->unit),
                glane_quality_name(s->quality), (unsigned)s->flags);
        p->published++;
        return 0;
    }
    /* Feuilles scalaires : l'etat COURANT (tronque a chaque ecriture). */
    snprintf(buf, sizeof buf, "%d\n", (int)s->value_milli);
    put_leaf(p, 0, buf);
    snprintf(buf, sizeof buf, "%s\n", glane_quality_name(s->quality));
    put_leaf(p, 1, buf);
    snprintf(buf, sizeof buf, "%u\n", (unsigned)s->seq);
    put_leaf(p, 2, buf);
    snprintf(buf, sizeof buf, "%lld\n", (long long)s->ts_ns);
    put_leaf(p, 3, buf);
    snprintf(buf, sizeof buf, "%s\n", glane_unit_name(s->unit));
    put_leaf(p, 4, buf);
    /* Journal `info` : appende, puis borne (les N dernieres lignes). */
    snprintf(buf, sizeof buf,
             "{\"s\":\"%s\",\"id\":%u,\"seq\":%u,\"ts_ns\":%lld,\"v_m\":%d,"
             "\"unit\":\"%s\",\"quality\":\"%s\"}\n",
             p->desc->name, (unsigned)p->desc->id, (unsigned)s->seq,
             (long long)s->ts_ns, (int)s->value_milli,
             glane_unit_name(s->unit), glane_quality_name(s->quality));
    put(p->fds[5], buf);
    p->info_bytes += (long)strlen(buf);
    if (p->info_bytes > INFO_MAX_BYTES) {
        compact_info(p);
    }
    p->published++;
    return 0;
}

void glane_publish_close(glane_publisher *p)
{
    if (p == NULL) {
        return;
    }
    for (int i = 0; i < GLANE_PUBLISH_LEAVES; i++) {
        if (p->fds[i] >= 0) {
            close(p->fds[i]);
            p->fds[i] = -1;
        }
    }
    p->use_files = 0;
    p->opened = 0;
}