#!/bin/ksh
# Glane — Copyright 2026 Baptiste Faisy. Licence Apache-2.0.
#
# invite.sh — invite interactive de la VITRINE WEB de Glane, sur la console VGA
# de l'invité QNX 8.0.
#
# Pourquoi un script ? Le shell de démarrage `[+script]` d'une IFS QNX ne sait
# ni pipes ni redirections : impossible d'y écrire `ksh -i < /dev/con1`. On
# écrit donc ce script et on le lance par `ksh /proc/boot/invite.sh`.
#
# Pourquoi `< /dev/con1` ? `on -t /dev/con1 cmd` redirige la SORTIE de `cmd`
# vers la console VGA (vérifié : les mesures s'y affichent), mais le `ksh`
# lancé ainsi ne lit pas le clavier. En branchant explicitement son entrée
# sur le périphérique de console, la lecture des touches fonctionne (vérifié :
# la commande tapée est bien exécutée par l'invité).
#
# `ENV` est le fichier lu par `ksh` au démarrage d'un shell interactif : la
# bannière et l'invite « glane> » y sont écrites (le `ksh -i` efface l'écran
# au démarrage, donc la bannière doit être imprimée PAR le shell).
#
# La boucle relance l'invite si le shell sort : la vitrine reste vivante.

HOME=/proc/boot
export HOME
TERM=qansi
export TERM
ENV=/proc/boot/invite.kshrc
export ENV
PS1='glane> '
export PS1

while :; do
    ksh -i < /dev/con1
    sleep 1
done
