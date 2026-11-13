/*
 * Glane — Copyright 2026 Baptiste Faisy. Licence Apache-2.0.
 *
 * gsuper.c — superviseur (esprit de la supervision HAM, GLA-14) : relance un
 * programme dès qu'il se termine ou qu'il est tué. C'est un « HAM de poche » :
 * HAM (le vrai superviseur QNX) n'est pas dans le SDP installé, mais le
 * mécanisme démontré est le même — un publisher tué est relancé, la reprise
 * est comptée et datée.
 *
 * Usage : gsuper [-m max_reprises] <programme> [args...]
 *   -m N : nombre maximum de reprises (0 = illimité, défaut).
 *
 * Pourquoi la borne : sans elle, un superviseur qui surveille un programme
 * qui se termine de lui-même repasse en boucle sans fin (cas observé sur la
 * vitrine : 145 reprises en 24 minutes, CPU saturé). Avec -m, la supervision
 * est finie, vérifiable, et le dernier fils reste vivant.
 *
 * Se daemonise (fork) ; le père rend la main, le fils supervise.
 */
#define _QNX_SOURCE 1
#include <process.h>
#include <spawn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

int main(int argc, char **argv)
{
    int max_restarts = 0;   /* 0 = illimité */
    int idx = 1;

    if (argc > 2 && strcmp(argv[1], "-m") == 0) {
        max_restarts = atoi(argv[2]);
        if (max_restarts < 0) {
            fprintf(stderr, "gsuper: -m doit être >= 0\n");
            return 2;
        }
        idx = 3;
    }
    if (argc <= idx) {
        fprintf(stderr, "usage: %s [-m max_reprises] <programme> [args...]\n", argv[0]);
        return 2;
    }

    pid_t pid = fork();
    if (pid < 0) {
        return 1;
    }
    if (pid > 0) {
        printf("gsuper: surveille '%s' (pid superviseur %d)\n", argv[idx], (int)pid);
        fflush(stdout);
        return 0;
    }
    /* Fils superviseur : relance tant que l'enfant meurt (borne -m). */
    int restarts = 0;
    for (;;) {
        pid_t child = spawnv(P_WAIT, argv[idx], &argv[idx]);
        if (child == -1) {
            printf("gsuper: spawn '%s' KO (errno %d)\n", argv[idx], errno);
            fflush(stdout);
            sleep(1);
            continue;
        }
        restarts++;
        printf("gsuper: '%s' terminé (reprise #%d)\n", argv[idx], restarts);
        fflush(stdout);
        if (max_restarts > 0 && restarts >= max_restarts) {
            printf("gsuper: limite de reprises atteinte (%d) — arrêt du superviseur\n",
                   max_restarts);
            fflush(stdout);
            return 0;
        }
        sleep(1);
    }
}
