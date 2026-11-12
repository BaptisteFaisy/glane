/*
 * Glane — Copyright 2026 Baptiste Faisy. Licence Apache-2.0.
 *
 * gwatch.c — abonné de démonstration : relit en continu la dernière mesure
 * publiée d'un capteur (fichier value dans l'espace de noms) et lève une
 * alarme dès qu'elle dépasse un seuil. Boucle la démo publish/subscribe :
 * glaned publie, gwatch consomme et réagit.
 *
 * Usage : gwatch <fichier.value> <seuil_milli> [iterations]
 */
#define _QNX_SOURCE 1
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(int argc, char **argv)
{
    if (argc < 3) {
        fprintf(stderr, "usage: %s <fichier.value> <seuil_milli> [iter]\n", argv[0]);
        return 2;
    }
    const char *path = argv[1];
    long thresh = strtol(argv[2], NULL, 10);
    int iters = argc > 3 ? atoi(argv[3]) : 30;
    for (int i = 0; i < iters; i++) {
        FILE *f = fopen(path, "rb");
        long last = 0, v;
        if (f != NULL) {
            while (fscanf(f, "%ld", &v) == 1) {
                last = v;
            }
            fclose(f);
        }
        printf("gwatch[%s] value=%ld", path, last);
        if (last > thresh) {
            printf("  ALARME (seuil=%ld)", thresh);
        }
        printf("\n");
        fflush(stdout);
        usleep(100000);   /* 100 ms */
    }
    return 0;
}