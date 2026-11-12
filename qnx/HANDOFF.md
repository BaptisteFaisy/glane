# Passation — faire tourner `glaned` dans la VM QSTI QNX 8.0 et capturer la démo

> À l'attention de la session qui pilote la VM QSTI QEMU (qemu PID sur
> `~/qnx800/images/qemu/qemu/output/disk-qemu.vmdk`, série TCP 127.0.0.1:4444,
> ssh hostfwd 127.0.0.1:8022). Moi (session « site ») j'intègre la sortie au site
> **uniquement** — je ne touche pas à la VM.

## Ce qui est déjà prêt
- **`glaned` compilé pour QNX 8.0 x86_64** : `~/Bureau/glane/build-qnx/glaned`
  (ELF, interpréteur `/usr/lib/ldqnx-64.so.2`, `qcc -V gcc_ntox86_64 -std=c11 -O2
  -Wall -Wextra`, exit 0). Voir `qnx/capture/build-proof.txt`.
- Une description de capteur valide : `~/Bureau/glane/core/tests/valid/temp-chassis.json`.
- `glaned <desc.json> <steps> <waveform 0-4> <seed>` imprime le journal (une ligne
  JSON par mesure, format CONTRACT §7) sur **stdout** + un résumé sur stderr.
  waveform : 0=const 1=sine 2=ramp 3=triangle 4=noise.
  (La publication `/dev/sensors` est encore à l'état d'échafaudage dans
  `publish/qnx/glane_publish.c` → pour l'instant glaned écrit sur stdout, c'est
  ce qu'on capture.)

## Ce qu'il faut faire dans la VM (via ssh :8022 ou la console série :4444)
```sh
# 1) pousser glaned + une description dans l'invité (depuis l'hôte)
scp -P 8022 -o StrictHostKeyChecking=no \
    ~/Bureau/glane/build-qnx/glaned \
    ~/Bureau/glane/core/tests/valid/temp-chassis.json \
    root@127.0.0.1:/tmp/

# 2) dans l'invité : preuve d'OS + exécution
ssh -p 8022 root@127.0.0.1 '
  uname -a;                       # doit montrer QNX ... 8.0.0
  cat /proc/version 2>/dev/null;
  chmod +x /tmp/glaned;
  /tmp/glaned /tmp/temp-chassis.json 600 1 42
'
```

## Où déposer la sortie (pour que la session « site » l'intègre)
Mettre les artefacts dans **`~/Bureau/glane/qnx/capture/`** avec ces noms exacts :

| Fichier | Contenu |
|---|---|
| `runtime-uname.txt` | `uname -a` + version QNX **depuis l'invité** (preuve d'exécution sur QNX) |
| `runtime-glaned.txt` | stdout+stderr complets de `glaned` lancé **dans l'invité** |
| `runtime-journal.jsonl` | les lignes JSON produites par glaned dans l'invité (son stdout) |
| `runtime-howto.md` | la commande qemu exacte + les commandes tapées + date + qui |
| `console-*.png` (optionnel) | captures de la console/écran QNX (ex. via le moniteur qemu `screendump`) |

Dès que `runtime-*.txt`/`.jsonl` existent, la session « site » régénère
`site/data.js` (`python3 site/gen_data.py`) et la section **« Émulateur QNX »**
affiche la sortie réelle. `gen_data.py` les embarque automatiquement s'ils sont
présents (sinon la section affiche la preuve de compilation seule).

## Vérifications utiles à capturer (bonus, « Evidence of QNX use »)
- `uname -a` dans l'invité (QNX 8.0.0).
- La commande qemu complète (pour reproductibilité).
- `pidin` ou `ls /dev/sensors` (si/une fois la publication PPS implémentée).
- Le fait que **deux runs de glaned avec la même graine donnent le même journal**
  (rejeu déterministe, GLA-04) — lancer deux fois et `cmp`.
