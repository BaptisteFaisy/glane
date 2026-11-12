/*
 * Glane — Copyright 2026 Baptiste Faisy. Licence Apache-2.0.
 *
 * gsensor_resmgr.c — gestionnaire de ressources QNX : expose la dernière
 * mesure publiée d'un capteur comme un vrai nœud /dev/sensors/<nom>.
 *
 *   cat /dev/sensors/<nom>   ->  valeur courante (dernière ligne du fichier
 *                                « value » publié par glaned)
 *
 * C'est la voie « device publishers » de QNX (resmgr + iofunc), et non un
 * simple fichier : le nœud existe dans l'espace de noms des périphériques, il
 * s'ouvre/se lit comme un device, et il est donc accessible par n'importe quel
 * client QNX (et par les tools QNX standard).
 *
 * Usage : gsensor_resmgr <nom> <fichier-value>
 */
#define _QNX_SOURCE 1
#include <sys/iofunc.h>
#include <sys/dispatch.h>
#include <sys/resmgr.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <unistd.h>

typedef struct {
    char name[64];
    char value_file[256];
} sensor_resmgr_t;

static sensor_resmgr_t g_ctx;
static resmgr_connect_funcs_t g_connect;
static resmgr_io_funcs_t g_io;
static iofunc_funcs_t g_iofunc = {
    _IOFUNC_NFUNCS,
    iofunc_ocb_calloc,
    iofunc_ocb_free,
    NULL, NULL, NULL
};
static iofunc_mount_t g_mount;
static iofunc_attr_t g_attr;
static resmgr_attr_t g_rattr;
static dispatch_t *g_dpp;

/* Lit la dernière valeur publiée (dernière ligne du fichier value). */
static long read_last_value(const char *path)
{
    FILE *f = fopen(path, "rb");
    long last = 0, v;
    if (f == NULL) {
        return 0;
    }
    while (fscanf(f, "%ld", &v) == 1) {
        last = v;
    }
    fclose(f);
    return last;
}

static int my_read(resmgr_context_t *ctp, io_read_t *msg, RESMGR_OCB_T *ocb)
{
    char buf[64];
    int len = snprintf(buf, sizeof buf, "%ld\n",
                       read_last_value(g_ctx.value_file));
    off_t off = ocb->offset;
    size_t nleft = (size_t)((off < len) ? (len - off) : 0);
    if (nleft == 0) {
        /* fin de fichier : plus rien à lire */
        _IO_SET_READ_NBYTES(ctp, 0);
        msg->i.nbytes = 0;
        return EOK;
    }
    size_t n = msg->i.nbytes;
    if (n > nleft) {
        n = nleft;
    }
    SETIOV(ctp->iov, buf + off, (int)n);
    ocb->offset = off + (off_t)n;
    msg->i.nbytes = 0;
    _IO_SET_READ_NBYTES(ctp, n);
    return _RESMGR_NPARTS(1);
}

static int setup(const char *name, const char *value_file)
{
    snprintf(g_ctx.name, sizeof g_ctx.name, "%s", name);
    snprintf(g_ctx.value_file, sizeof g_ctx.value_file, "%s", value_file);

    iofunc_mount_init(&g_mount, sizeof g_mount);
    g_mount.funcs = &g_iofunc;
    iofunc_attr_init(&g_attr, S_IFREG | 0666, NULL, NULL);
    g_attr.mount = &g_mount;
    g_attr.nbytes = 64;

    iofunc_func_init(_RESMGR_CONNECT_NFUNCS, &g_connect,
                     _RESMGR_IO_NFUNCS, &g_io);
    g_io.read = my_read;

    memset(&g_rattr, 0, sizeof g_rattr);
    g_rattr.nparts_max = 1;
    g_rattr.msg_max_size = 2048;

    g_dpp = dispatch_create();
    if (g_dpp == NULL) {
        printf("gsensor_resmgr: dispatch_create KO (errno %d)\n", errno);
        fflush(stdout);
        return -1;
    }
    char path[256];
    snprintf(path, sizeof path, "/dev/sensors/%s", g_ctx.name);
    if (resmgr_attach(g_dpp, &g_rattr, path, _FTYPE_ANY, 0,
                      &g_connect, &g_io, &g_attr) == -1) {
        printf("gsensor_resmgr: resmgr_attach KO (%d): %s\n", errno, strerror(errno));
        fflush(stdout);
        return -1;
    }
    printf("gsensor_resmgr: ATTACH OK %s (depuis %s)\n", path, g_ctx.value_file);
    fflush(stdout);
    return 0;
}

static int serve_forever(void)
{
    dispatch_context_t *ctp = dispatch_context_alloc(g_dpp);
    if (ctp == NULL) {
        perror("dispatch_context_alloc");
        return 1;
    }
    for (;;) {
        dispatch_block(ctp);
        dispatch_handler(ctp);
    }
}

int main(int argc, char **argv)
{
    if (argc < 3) {
        fprintf(stderr, "usage: %s <nom> <fichier.value>\n", argv[0]);
        return 2;
    }
    /* Fork : le parent rend la main (le script de démarrage continue), le fils
     * attache /dev/sensors/<nom> et sert les lectures. */
    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        return 1;
    }
    if (pid > 0) {
        fprintf(stderr, "gsensor_resmgr: /dev/sensors/%s (pid %d)\n", argv[1], (int)pid);
        return 0;
    }
    if (setup(argv[1], argv[2]) != 0) {
        return 1;
    }
    return serve_forever();
}