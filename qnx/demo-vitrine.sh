#!/bin/ksh
# Glane — Copyright 2026 Baptiste Faisy. Licence Apache-2.0.
#
# demo-vitrine.sh — tableau de bord de démonstration PEINT SUR L'ÉCRAN de
# l'invité (console VGA con1). À chaque cycle, l'écran est effacé puis repeint
# avec l'état RÉEL lu dans le système : valeur publiée, séquence, objet du
# gestionnaire de ressources, les trois capteurs, l'abonné, les processus.
# Rien n'est simulé ni écrit à la main : chaque ligne sort d'une commande
# exécutée dans QNX 8.0.
#
# Pourquoi si court : l'écran VGA de l'invité tient 25 lignes. La première
# version déversait ~28 lignes par cycle — le bandeau sortait de l'écran.
# Ici : 17 lignes, il reste de la place pour la frappe du visiteur.
#
# Pause (le visiteur veut son shell tranquille) :
#       echo pause > /dev/shmem/arret
# Reprise :
#       rm /dev/shmem/arret
#
# Lancé par qnx/glane-vitrine.build :
#       on -d -t /dev/con1 ksh /proc/boot/demo-vitrine.sh
# Le shell interactif (qnx/invite.sh) tourne sur la même console.

V=/dev/shmem/temp-chassis
N=0
while :; do
    if [ -e /dev/shmem/arret ]; then
        sleep 2
        continue
    fi
    N=$((N + 1))
    clear
    printf ' GLANE - QNX %s - cycle #%s\n' "$(uname -r)" "$N"
    printf '==================================================\n'
    printf ' capteur : temp-chassis (driver tmp, source virtuelle)  seq %s\n' \
        "$(cat $V.seq)"
    printf ' valeur  : %s milli-C   qualite %s\n' "$(cat $V.value)" "$(cat $V.quality)"
    printf ' resmgr  : cat /dev/sensors/temp-chassis -> %s\n' \
        "$(cat /dev/sensors/temp-chassis)"
    printf '--- 3 capteurs, 3 cadences --------------------------\n'
    for s in temp-chassis imu-accel batt-voltage; do
        printf ' %-14s %s\n' "$s" "$(cat /dev/shmem/$s.value)"
    done
    printf '--- abonne (seuil 60000 milli) ----------------------\n'
    gwatch $V.value 60000 1
    printf '--- processus QNX -----------------------------------\n'
    pidin -F '%n' | head -n 4 | tail -n 3
    printf '==================================================\n'
    printf ' Tapez une commande -> reponse ici :\n'
    printf ' ls /dev/shmem | cat /dev/sensors/temp-chassis\n'
    printf ' Pause : echo pause > /dev/shmem/arret\n'
    printf '==================================================\n'
    sleep 8
done
