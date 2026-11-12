/*
 * Glane — Copyright 2026 Baptiste Faisy. Licence Apache-2.0.
 *
 * qnxproof.c — preuve d'exécution sur la cible : imprime uname(2) (sysname,
 * release, version, machine, node) plus un marqueur passé en argument.
 * Compilé en STATIQUE (qcc -static) pour tenir dans une IFS minimale sans
 * aucune bibliothèque partagée. Portable : fonctionne aussi sur Linux.
 *
 * Usage : qnxproof ["<marqueur>"]
 */
#include <stdio.h>
#include <sys/utsname.h>

int main(int argc, char **argv)
{
    struct utsname u;
    if (uname(&u) == 0) {
        printf("[QNX-PROOF] sysname=%s release=%s version=%s machine=%s node=%s\n",
               u.sysname, u.release, u.version, u.machine, u.nodename);
    } else {
        printf("[QNX-PROOF] uname() a échoué\n");
    }
    if (argc > 1) {
        printf("[MARK] %s\n", argv[1]);
    }
    fflush(stdout);
    return 0;
}