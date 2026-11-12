# Glane sur QNX 8.0 — runbook émulateur (QSTI QEMU)

> ⚠️ **Coordination multi-sessions** : plusieurs projets (Veille, Éclat, Toile,
> Glane) partagent le même SDP `~/qnx800` et la même image QSTI. Avant toute
> action QNX (extraction, boot de VM, arrêt), consulter le hub
> **`~/Bureau/.scratch/qnx-coord/`** (`PROCESS.md` = règles, `qnx-coord.sh status`
> = VMs/verrous/ports, `registry.tsv` = qui possède quoi). Règle absolue :
> **jamais `pkill -f qemu-system`** (tue les VMs des autres) — tuer par PID/port.

Objectif : compiler `glaned` pour QNX 8.0, le faire tourner dans l'image
**Quick Start Target Image (QSTI) pour QEMU**, et **capturer la sortie réelle**
(journal, latences, arbre `/dev/sensors`, console) pour la section « Émulateur »
du site. Toutes les commandes de la partie QNX proviennent de la doc officielle
QNX Everywhere (mise à jour 2026-09-24).

## 0. Prérequis (action humaine — compte myQNX)

1. Licence **QNX Everywhere** non-commerciale demandée + acceptée (QDL 2.0) sur
   https://www.qnx.com/getqnx → email de disponibilité sous ~30 min (jusqu'à 1 h).
2. Dans le **myQNX License Manager** (https://www.qnx.com/account/dashboard/) :
   **Accept License** puis **Deploy** vers son propre compte (email + Save).
3. Télécharger + installer le **QNX Software Center (QSC)** :
   https://www.qnx.com/download/group.html?programid=29178
4. Dans QSC : installer **QNX SDP 8.0** (défaut `~/qnx800`) **et** le paquet
   **`com.qnx.qnx800.quickstart.qemu`** (l'image QSTI QEMU) → `~/qnx800/images/qemu`.

Hôte déjà prêt ici : `qemu-system-x86_64` 8.2.2, `/dev/kvm`, Linux Mint 22.3
(base Ubuntu 24.04, supportée). QEMU 8 = pas de Vulkan (inutile pour une démo
console/série). Espace : prévoir ~15 Go (SDP + image).

## A. Compiler glaned pour QNX

```sh
qnx/build.sh                 # -> build-qnx/glaned  (source qnxsdp-env.sh, qcc -V gcc_ntox86_64le)
```

Le socle `core/` est du C11 portable pur : il compile tel quel avec `qcc`.
Seuls `target/glaned.c` et `publish/qnx/` touchent à l'OS. La variante de
toolchain (`gcc_ntox86_64le`) est à confirmer avec `qcc -V` une fois le SDP là.

> ⚠️ À ce stade, `publish/qnx/glane_publish.c` est un **échafaudage** : les
> blocs `#ifdef __QNX__` décrivent les appels PPS/resource-manager à écrire
> (créer `/dev/sensors/<name>/{value,quality,seq,ts_ns,unit,info}` + `command`,
> notifier les abonnés). Sans eux, glaned retombe sur une ligne compacte sur
> stdout — suffisant pour prouver l'exécution sur QNX, à compléter pour la
> publication réelle (GLA-12).

## B. Lancer l'image QSTI sous QEMU

```sh
. ~/qnx800/qnxsdp-env.sh
cd ~/qnx800/images/qemu
sh unpack_qemu_image.sh          # dépaquette les qnx_sdp8.0_qemu_quickstart_*.tar.gz.*
cd qemu
mkqnximage --run                 # démarre la VM (8 CPU, 4 Go, 720p, sans Vulkan)
```

Dans un **autre** terminal (même dossier runtime) :
```sh
mkqnximage --getip               # IP de l'invité
mkqnximage --stop                # arrêt propre (sinon Ctrl+A puis x dans la console)
```

## C. Pousser glaned dans l'invité et l'exécuter

Deux options selon ce que l'image QSTI expose (sshd et/ou partage) :

**Option 1 — SSH/scp** (si l'invité a un sshd) :
```sh
IP=$(cd ~/qnx800/images/qemu/qemu && mkqnximage --getip)
scp -o StrictHostKeyChecking=no build-qnx/glaned core/tests/valid/temp-chassis.json root@$IP:/tmp/
ssh root@$IP "chmod +x /tmp/glaned && /tmp/glaned /tmp/temp-chassis.json 600 1 42"
```

**Option 2 — partage de l'image** : déposer `glaned` + la description dans le
répertoire partagé de la QSTI (selon le « Getting started » QSTI QEMU), puis les
exécuter depuis la console de l'invité.

## D. Capturer la démo (ce qui va sur le site)

- **Journal + latence** : la sortie de `glaned` (ou un journal produit sur
  l'invité) → `host/glat.py` (p50/p99/p99.9) et `host/greport.py` (HTML).
- **Arbre publié** : `ls -R /dev/sensors` sur l'invité → `host/gschema.py`
  (validateur de conformité, GLA-08).
- **Console/écran** : capture du framebuffer QNX (la QSTI a une sortie VGA ;
  `qnx_console.py` du projet voisin `~/Bureau/qnx71-vm` montre le pattern
  screendump via le moniteur QEMU).
- **Preuve d'exécution** : `uname -a` sur l'invité (QNX 8.0), version du SDP,
  variante `qcc -V` — exigées par le règlement (« Evidence of QNX use »).

## E. Câbler dans le site

Ajouter une section « Émulateur QNX » qui affiche la sortie **réelle** capturée
(journal QNX, `glat`, `gschema`, screenshots) à côté de la démo JS. Mettre à
jour `site/gen_data.py` pour embarquer ces artefacts, régénérer `site/data.js`,
republier `gh-pages`.

## F. Image custom (optionnel, pour l'autostart)

Pour embarquer `glaned` au démarrage d'une image (au lieu de le pousser à la
main) : voir `image/mkifs.fragment` + le **Custom Target Image (CTI) Guide**
(`com.qnx.doc.target_images/topic/cti/about.html`). Superviser `glaned` par
**HAM** (GLA-14) : un publisher tué doit être relancé, reprise mesurée.

## État de l'échafaudage

| Élément | État |
|---|---|
| `target/glaned.c` | écrit, **compile sur Linux** (repli stdout), boucle testée |
| `publish/qnx/glane_publish.{h,c}` | interface + repli portable écrits ; **PPS/resmgr QNX = TODO** (blocs `#ifdef` documentés) |
| `qnx/build.sh` | écrit ; **à valider** avec `qcc` réel (variante toolchain) |
| `image/glaned.buildfile`, `image/mkifs.fragment` | templates ; **à valider** avec le SDP |
| `qnx/run-and-capture` | à écrire une fois l'invité QSTI joignable (ssh vs partage) |

Rien ici ne casse le build Linux : la CI (`make test`, `make asan`, `make
sizecheck`, `make host-test`) reste verte, le code QNX étant confiné derrière
`#ifdef __QNX__` et hors des cibles compilées par le Makefile hôte.