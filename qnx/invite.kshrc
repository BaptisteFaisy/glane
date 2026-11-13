# Glane — Copyright 2026 Baptiste Faisy. Licence Apache-2.0.
#
# invite.kshrc — fichier de démarrage du shell interactif de la vitrine
# (lu par `ksh -i` via la variable ENV, cf. qnx/invite.sh).
#
# Il est imprimé APRÈS le démarrage du shell (le `ksh -i` efface l'écran au
# lancement) : c'est donc la bannière visible sur l'écran VGA de la vitrine.

PS1='glane> '

echo "=============================================================="
echo " QNX 8.0 - INVITE GLANE"
echo "=============================================================="
echo " glaned publie ses mesures dans l'espace de noms de l'invite."
echo " Derniere mesure publiee (temp-chassis) :"
cat /dev/shmem/temp-chassis.info 2>/dev/null
echo " Exemples : ls /dev/shmem ; cat /dev/shmem/imu-accel.info"
echo "=============================================================="
