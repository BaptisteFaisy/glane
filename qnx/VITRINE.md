# Vitrine web de Glane — invité QNX 8.0 en direct

Ce dossier décrit la **vitrine** de Glane : un invité QNX 8.0 réel sous QEMU,
exposé sur le web (écran VGA en direct, console série, injection de touches),
plus une page sur le site du dossier QNX.

## URL

```
https://baptiste-ms-7d18.tail3a8bdf.ts.net:10000
```

- service HTTP local : `http://127.0.0.1:8101` (port réservé, hors des ports
  des autres projets : 4444, 4445, 4450, 2280, 8022) ;
- tunnel : `tailscale funnel --bg --https=10000 http://127.0.0.1:8101`
  (seuls 443, 8443 et 10000 sont servis par l'entrée funnel : **8101 ne
  fonctionne pas**, 8443 est déjà pris par un autre projet de cette machine) ;
- page du site du dossier QNX : `docs/glane.html` (et `docs/glane-demo.html`).

## Services systemd (utilisateur)

| Unité | Rôle |
|---|---|
| `~/.config/systemd/user/glane-qemu.service` | l'invité QEMU (PID dans le registre `qnx-coord`) |
| `~/.config/systemd/user/glane-web.service` | la vitrine HTTP (`web.py` de `qnx-emu`) |

Copies versionnées dans `qnx/systemd/`. Démarrage :

```sh
systemctl --user daemon-reload
systemctl --user enable --now glane-qemu.service glane-web.service
```

## Artefacts

| Chemin | Rôle |
|---|---|
| `build-qnx/glane-vitrine-ifs.bin` | l'IFS amorcée (`-kernel`) |
| `build-qnx/vitrine/serial.log` | console série (journal de la vitrine) |
| `build-qnx/vitrine/mon.sock` | socket du moniteur QEMU (`screendump`, `sendkey`) |
| `build-qnx/vitrine/web-vga.ppm` | capture VGA brute (convertie en PNG par `web.py`) |

## Reconstruire l'IFS

```sh
cd ~/Bureau/glane
. ~/qnx800/qnxsdp-env.sh
mkifs qnx/glane-vitrine.build build-qnx/glane-vitrine-ifs.bin
systemctl --user restart glane-qemu.service
```

## Ce que fait l'invité

`qnx/glane-vitrine.build` reprend le scénario de `qnx/glane-ifs.build` (la
preuve, qui reste intacte) et y ajoute **les deux consoles** :

- la console **série** (`con0`, mise en place par `startup-x86 -D8250..115200`)
  joue le scénario : `uname`, `pidin`, `glaned` qui publie `temp-chassis`,
  `imu-accel` et `batt-voltage` dans `/dev/shmem`, `gwatch` (abonné + alarme),
  `gsensor_resmgr` (`/dev/sensors`), `gsuper` (relance de `glaned`) ;
- la console **VGA** (`con1`, ouverte par `devc-con`) montre les mesures puis
  l'invite `ksh` interactive (`qnx/invite.sh` + `qnx/invite.kshrc`).

## Pièges payés (ne pas les rediscover)

1. **`exec ksh -l` ne marche pas** dans le `[+script]` de démarrage :
   « Unable to start "exec" (2) » — ce n'est pas un shell. Passer par `on`.
2. **`on -t /dev/con1 cmd` redirige la sortie** (vérifié : les mesures
   s'affichent sur la VGA) **mais le `ksh` lancé ainsi ne lit pas le clavier** :
   il faut brancher son entrée sur le périphérique de console —
   `ksh -i < /dev/con1` — impossible directement dans le `[+script]`, d'où le
   script `qnx/invite.sh`.
3. **`ksh -i` efface l'écran au démarrage** : la bannière doit être imprimée
   *par* le shell, donc depuis son fichier de démarrage (`$ENV`, ici
   `qnx/invite.kshrc`), et non avant de le lancer.
4. **QEMU de la distribution Debian** : `-L /usr/share/qemu` casse la
   recherche du BIOS (Debian a plusieurs répertoires de données). Les liens
   sympathiques sont dans `~/.qemu-data/usr/share/qemu/` (passés via
   `--prefixe`).
5. **Le clavier fonctionne** : `sendkey` du moniteur QEMU atteint bien
   l'invité (`cat /dev/con1` affiche la frappe ; le shell exécute la commande).
   **Piège payé** : le moniteur ne renvoie pas de réponse par commande ; la
   boucle de lecture de `web.py` attendait donc son timeout (3,3 s PAR touche).
   Une commande de 20 caractères partait en morceaux (le client abandonnait
   avant la fin). D'où `envoyer_touches()` : **une seule connexion**, les
   `sendkey` s'enchaînent à 60 ms, sans lire de réponse — 20 touches en ~1,2 s.
6. **Envoyer une ligne de commande** : `GET /type?t=<texte>&ret=1&delay=0.06`
   tape la chaîne touche par touche (majuscules et symboles traduits en
   `shift-…`), `ret=1` ajoute Entrée. La page de la vitrine et la section
   « Émulateur QNX » du site ont un champ de saisie branché dessus ; la réponse
   du `ksh` apparaît sur l'écran VGA (relu par `/screen.png` ou `/stream`).
   Exemple : `curl 'http://127.0.0.1:8101/type?t=ls%20/dev/shmem'`.
6. Coordination multi-projets : ports réservés via
   `qnx-coord.sh vm-add glane …`, arrêt **par PID** uniquement, jamais de
   `pkill qemu`.
