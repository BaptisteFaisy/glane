# Runner self-hosted du job QNX (`pc-qnx-repo`)

Le job `QNX Build` ne peut tourner que sur ce poste : c'est le seul endroit où
le SDP QNX 8.0 est installé et licencié. Il est lancé par un runner GitHub
Actions auto-hébergé, en service systemd utilisateur.

## État (13/11/2026)

| Élément | Valeur |
|---|---|
| Nom | `pc-qnx-repo` (id 22, **au niveau du dépôt** `Tradoooo/glane`) |
| Libellés | `self-hosted`, `Linux`, `X64`, `qnx` (le workflow demande `[self-hosted, qnx]`) |
| Répertoire | `~/.glane-runner2` |
| Unité | `glane-runner-repo.service` (copie versionnée dans `qnx/systemd/`) |
| Version du runner | 2.338.2 |

## Piège n°1 — runner d'organisation « offline/busy » (split-brain)

Le runner d'origine (`pc-qnx`, id 21) était enregistré **au niveau de
l'organisation** `Tradoooo`. Un redémarrage de service l'a tué **en plein job** :
GitHub a gardé la session, le processus dit « Listening for Jobs » mais l'API
répond `status: offline, busy: true` → plus aucun job n'est pris, et le job en
cours reste « in progress » pour toujours. `systemctl --user restart` ne suffit
pas. Le retrait propre demande un jeton `remove-token` **org** (`admin:org`),
que le compte n'a pas.

Solution retenue : enregistrer un runner **au niveau du dépôt** (jeton
`repos/Tradoooo/glane/actions/runners/registration-token`, portée `repo`), avec
les mêmes libellés. Il prend les jobs tout de suite, sans toucher à
l'organisation. L'ancien runner d'organisation reste « offline » (inoffensif).

```sh
# (une seule fois) enregistrement
cd ~/.glane-runner2
./config.sh --unattended --url https://github.com/Tradoooo/glane \
  --token "$(gh api -X POST repos/Tradoooo/glane/actions/runners/registration-token --jq .token)" \
  --labels qnx --name pc-qnx-repo --work _work
systemctl --user enable --now glane-runner-repo.service
```

## Piège n°2 — bibliothèques du SDP zéroées par un processus de l'hôte

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