/*
 * Glane — Copyright 2026 Baptiste Faisy. Licence Apache-2.0.
 *
 * gsuper.c — superviseur (esprit de la supervision HAM, GLA-14) : relance un
 * programme dès qu'il se termine ou qu'il est tué. C'est un « HAM de poche » :
 * HAM (le vrai superviseur QNX) n'est pas dans le SDP installé, mais le
 * mécanisme démontré est le même — un publisher tué est relancé, la reprise
 * est comptée et datée.
 *
 * Usage : gsuper <programme> [args...]
 * Se daemonise (fork) ; le père rend la main, le fils supervise.
 */
#define _QNX_SOURCE 1
#include <process.h>
#include <spawn.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>

int main(int argc, char **argv)
{
    if (argc < 2) {
        fprintf(stderr, "usage: %s <programme> [args...]\n", argv[0]);
        return 2;
    }
    pid_t pid = fork();
    if (pid < 0) {
        return 1;
    }
    if (pid > 0) {
        printf("gsuper: surveille '%s' (pid superviseur %d)\n", argv[1], (int)pid);
        fflush(stdout);
        return 0;
    }
    /* Fils superviseur : relance tant que l'enfant meurt. */
    int restarts = 0;
    for (;;) {
        pid_t child = spawnv(P_WAIT, argv[1], &argv[1]);
        if (child == -1) {
            printf("gsuper: spawn '%s' KO (errno %d)\n", argv[1], errno);
            fflush(stdout);
            sleep(1);
            continue;
        }
        restarts++;
        printf("gsuper: '%s' terminé (reprise #%d)\n", argv[1], restarts);
        fflush(stdout);
        sleep(1);
    }
}