# Runner self-hosted du job QNX (`pc-qnx-repo`)

Le job `QNX Build` ne peut tourner que sur ce poste : c'est le seul endroit où
le SDP QNX 8.0 est installé et licencié. Il est lancé par un runner GitHub
Actions auto-hébergé, en service systemd utilisateur.

## État (13/11/2026)

| Élément | Valeur |
|---|---|
| Nom | `pc-qnx-repo` (id 23, **au niveau du dépôt** `Tradoooo/glane`) |
| Libellés | `self-hosted`, `Linux`, `X64`, `qnx`, **`glane-qnx`** |
| Répertoire | `~/.glane-runner2` |
| Unité | `glane-runner-repo.service` (copie versionnée dans `qnx/systemd/`) |
| Script de lancement | `~/.glane-runner2/run.sh` (versionné : `qnx/systemd/glane-runner-repo-run.sh`) |
| Version du runner | 2.338.2 |

## Piège n°1 — le label `qnx` ne suffit pas (runner d'organisation pris par défaut)

Le workflow demande `runs-on: [self-hosted, qnx]`. Or le libellé `qnx` est
aussi porté par le runner **d'organisation** `pc-qnx` (id 21), qui tourne sur
le **r630** : il a le SDP QNX mais **pas QEMU**. Le job était donc pris par
le r630 et échouait sur « `qemu-system-x86_64` : commande introuvable » —
alors que le runner du dépôt (sur le poste, qui a QEMU) était libre.

Le libellé `glane-qnx`, **dédié au dépôt**, lève l'ambiguïté : le workflow
demande `[self-hosted, glane-qnx]` et seul `pc-qnx-repo` peut le prendre.

```sh
# ajouter le libellé dédié (une fois)
gh api -X POST repos/Tradoooo/glane/actions/runners/23/labels \
  --input - <<< '{"labels": ["glane-qnx"]}'
```

Le runner a aussi été **supprimé** du poste par un nettoyage d'un autre agent
(`~/.glane-runner2` et `~/.glane-runner` ont disparu) : il a fallu le
réenregistrer (jeton `registration-token`, portée `repo`). Id 22 → id 23.

## Piège n°2 — le PATH du service doit contenir `/usr/bin`

Le service systemd lance le runner avec un PATH minimal : sans `/usr/bin`,
QEMU n'est pas trouvé et le job échoue sur une erreur trompeuse. D'où le
script `run.sh` versionné, qui exporte le PATH avant de lancer le runner.

## Piège n°3 — runner d'organisation « offline/busy » (split-brain)

Le runner d'origine (`pc-qnx`, id 21) était enregistré **au niveau de
l'organisation** `Tradoooo`. Un redémarrage de service l'a tué **en plein job** :
GitHub a gardé la session, le processus dit « Listening for Jobs » mais l'API
répond `status: offline, busy: true` → plus aucun job n'est pris, et le job en
cours reste « in progress » pour toujours. `systemctl --user restart` ne suffit
pas. Le retrait propre demande un jeton `remove-token` **org** (`admin:org`),
que le compte n'a pas.

Solution retenue : enregistrer un runner **au niveau du dépôt** (jeton
`repos/Tradoooo/glane/actions/runners/registration-token`, portée `repo`).
Il prend les jobs tout de suite, sans toucher à l'organisation.

```sh
# (une seule fois) enregistrement
cd ~/.glane-runner2
./config.sh --unattended --url https://github.com/Tradoooo/glane \
  --token "$(gh api -X POST repos/Tradoooo/glane/actions/runners/registration-token --jq .token)" \
  --labels qnx --name pc-qnx-repo --work _work
systemctl --user enable --now glane-runner-repo.service
```

## Piège n°4 — bibliothèques du SDP zéroées par un processus de l'hôte

Le 13/11 à 14:44, **9 fichiers du SDP** ont été remis à zéro (**taille 0,
mode 000**) par un processus de nettoyage du poste (non identifié ; le disque
n'était pas plein, 163 Go libres) :

```
target/qnx/x86_64/lib/gcc/12.2.0/{libgcc_s,libasan,liblsan,libubsan,libgomp,libquadmath,libstdc++}.so
target/qnx/x86_64/usr/bin/portmap
target/qnx/aarch64le/usr/bin/portmap
```

Symptôme côté compilation : `fatal error: sys/neutrino.h: No such file` ou
`yvals.h`, puis `cannot find -lgcc_s` — alors que les en-têtes existent. Un
`find ~/qnx800 -size 0 -newermt ...` les montre tout de suite.

Réparation (les vraies bibliothèques versionnées sont intactes) :

```sh
cd ~/qnx800/target/qnx/x86_64/lib/gcc/12.2.0
ln -sfn $HOME/qnx800/target/qnx/x86_64/lib/libasan.so.8.0.0      libasan.so
ln -sfn $HOME/qnx800/target/qnx/x86_64/lib/libgomp.so.1.0.0      libgomp.so
ln -sfn $HOME/qnx800/target/qnx/x86_64/lib/liblsan.so.0.0.0      liblsan.so
ln -sfn $HOME/qnx800/target/qnx/x86_64/lib/libquadmath.so.0.0.0  libquadmath.so
ln -sfn $HOME/qnx800/target/qnx/x86_64/lib/libstdc++.so.6.0.30   libstdc++.so
ln -sfn $HOME/qnx800/target/qnx/x86_64/lib/libubsan.so.1.0.0     libubsan.so
cp -f  $HOME/qnx800/target/qnx/x86_64/lib/libgcc_s.so.1 libgcc_s.so
```

Les deux `portmap` (x86_64 et aarch64le) restent à zéro : ils ne servent pas
au build de Glane. Pour eux, réinstaller le SDP via QNX Software Center.

## Vérifier

```sh
gh api repos/Tradoooo/glane/actions/runners --jq '.runners[] | {name,status,busy}'
gh workflow run qnx-build.yml --repo Tradoooo/glane --ref main
gh run list --repo Tradoooo/glane -L 2      # ~30 s, success
```