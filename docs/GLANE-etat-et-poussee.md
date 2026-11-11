# Glane — où cela en est, et comment le pousser au maximum

**Date de l'état : mardi 11 novembre 2026, 23 h 20 (Europe/Paris).**
**Échéance du concours : mardi 15 décembre 2026, 20 h (Paris). Il reste 33 jours.**
**Gel fonctionnel recommandé : vendredi 5 décembre (J-10).**

Cahier des charges complet : [`GLANE-cahier-des-charges.md`](GLANE-cahier-des-charges.md)
(324 lignes, 24 exigences GLA-01 à GLA-24, 4 essais de faisabilité, 9 lots délégables).
Ce document ne le remplace pas : il dit **où en est le projet** et **comment le pousser au maximum**.

---

## 1. État des lieux, sans complaisance

| Élément | État |
|---|---|
| Cahier des charges | **Écrit et publié** (dépôt public `BaptisteFaisy/qnx-dossier`) |
| Lignes de code produit | **0** |
| SDP QNX 8.0 installé | **Non** |
| Image Quick Start Raspberry Pi 5 téléchargée | **Non** |
| Essai E2 tranché (un processus tiers peut-il publier ses propres objets ?) | **Non — c'est le risque bloquant du projet** |
| Matériel (Raspberry Pi 5, capteur I2C) | **Non vérifié** |
| Vérifications documentaires faites | **Oui** : 10 trous sourcés dans le guide Device Publishers |

**Ce qui est certain** (pages lues : `dev_pub/about.html`, `overview.html`, `rmps_objects.html`,
`plugins.html`, `usblauncher_otg.html`, plus `gpio_framework/overview.html` et la technote SPI) :

- QNX livre un framework de publication de périphériques avec des objets dans une arborescence
  à **emplacements fixes**, et **un seul** publisher : `usblauncher_otg`, orienté USB, stockage
  et média (montage automatique via `rules.mnt`).
- **Aucune API ni SDK** pour écrire un publisher n'est documentée ; le guide ne décrit que le
  contenu des objets et les options en ligne de commande.
- **Aucune simulation ni test sans matériel** n'est mentionné dans tout le guide.
- **Aucun validateur de conformité** des objets publiés, alors que l'arborescence est imposée.
- **Rien** sur la latence de publication, l'horodatage des notifications ou la qualité de service.
- Le modèle de données est orienté média (mountpoint, partitions, TOC, température et mécanisme
  du disque) : **pas d'unités, pas de plages, pas de calibration, pas d'horodatage de capteur**.
- Les frameworks **GPIO et SPI sont documentés**, mais il n'existe **aucune abstraction capteur**
  au-dessus (découverte, calibration, fusion).
- Un mécanisme de plugins existe (generic, ipod, mediafs, audiocd, dvd) avec manager et
  *ratings* auto-déclarés — tous orientés média.
- Rien n'est documenté sur l'intégration des publishers au cycle de vie (HAM).
- **Le framework ne fait ni gestion ni arbitrage d'alarmes** (vérifié explicitement) : il n'y a
  pas de file prioritaire ni de préemption. Donc Glane ne recoupe pas un projet médical d'alarmes.

**Ce qui reste une hypothèse** : qu'un processus tiers puisse créer et publier ses propres
objets (essai E2) ; la disponibilité effective de l'I2C sur l'image Quick Start Raspberry Pi 5 ;
le comportement de HAM avec un publisher.

---

## 2. Rappel du contrat (extrait du cahier)

La mesure est figée à **24 octets** : `sensor_id`, `seq` monotone, `ts_ns` à la source,
`value_milli` en **milli-unités entières** (jamais de flottant), `unit`, `quality`
(OK/STALE/SATURATED/FAULT/UNCERTAIN), `flags` (FIRST/CALIBRATED/INJECTED/SIMULATED).

Deux choix à ne pas perdre : l'échelle entière permet au **jumeau Python** de rejouer bit à bit
la calibration ; `quality` décrit la mesure, `flags` décrit son origine — les mélanger rendrait
le rejeu ambigu.

Répartition des 24 exigences : **11 au niveau S**, **7 au niveau C**, **6 au niveau X**.

---

## 3. Comment le pousser au maximum — 25 leviers triés

Les 25 idées viennent de `qnx-100-aller-plus-loin.md` (lot 2) : 6 à effort faible, 16 moyen,
3 élevé ; 7 notées gain maximal.

### Palier 1 — cette semaine (avant le 17/11), effort faible

| Idée | Ce que ça ajoute | Pourquoi maintenant |
|---|---|---|
| **Démo chaos scénarisée** | 3 minutes : panne injectée en direct, dégradation déclarée visible, récupération automatique, latence affichée | La vidéo est obligatoire ; la scénariser avant d'écrire le code évite de découvrir qu'il ne montre rien |
| **`glane doctor`** | Diagnostic terrain en une commande : santé des bus, dernières mesures, compteurs d'erreurs, rapport collable dans un ticket | C'est ce qui transforme une bibliothèque en produit |
| **Journal d'audit immuable** | Trace horodatée non réécrivable des attachements, pannes, calibrations et changements de configuration | Exigence de traçabilité FDA et ISO 26262 — argument de marché direct |
| **Fiche de montée en charge** | Une commande produit le tableau p50/p99/p99.9 en fonction du nombre de capteurs | **Reproductible par un juge sur sa propre cible** : c'est rare et ça compte |
| **Intégrité bout en bout** | CRC par mesure, détection de trous de `seq`, compteurs de pertes par abonné | Faible coût, et cela répond au critère « implémentation non triviale » |
| **Fiche capteur générée** | Documentation du capteur produite depuis sa description JSON, avec courbes issues du rejeu | Livrable « documentation » des dossiers médicaux et automobiles |

### Palier 2 — avant le gel du 5 décembre, effort moyen

| Idée | Ce que ça ajoute | Prix visé |
|---|---|---|
| **Trace System Profiler** | Événements de traçage personnalisés émis vers `tracelogger` : le pipeline capteurs devient visible dans la vue noyau de Momentics | **Bonus « QNX-specific Technologies »** |
| **Détection de capteur menteur** | Variance nulle, saut physiquement impossible, plausibilité croisée entre capteurs corrélés, mise en quarantaine et propagation de `UNCERTAIN` | Innovation : aucun framework ne fait ça |
| **Oscilloscope web embarqué** | Gestionnaire de ressources servant un tableau de bord WebSocket sur la cible : courbes, qualité, percentiles, rejeu | **Vote Community Choice** |
| **Codelab QEMU auto-corrigé** | `glane lab` : lance QEMU avec chapitres guidés et vérification automatique de chaque étape | **Bonus 3 500 $ Codelab** |
| **Pont nomenclature médicale** | Correspondance des unités et grandeurs vers les codes MDC d'**IEEE 11073** | Preuve d'interopérabilité avec un standard d'un marché que QNX sert |
| **Duo Raspberry Pi 5 réel / virtuel** | Le même scénario joué sur un chapeau I2C réel puis en virtuel | Prouve que la simulation n'est pas un jouet — désarme l'objection principale |
| **Récupération de bus** | Repli exponentiel, mise sur liste noire du capteur fautif, réanalyse du bus, reconnexion transparente, événement publié | Fiabilité réelle, démontrable en vidéo |
| **Rechargement à chaud** | Reconfiguration sans redémarrage via `ionotify`, validation du schéma avant application, retour automatique à la configuration précédente | Critère « produit cohérent » |
| **Fusion multi-cadences** | Filtre complémentaire alignant IMU 100 Hz, température 1 Hz et encodeur sur une chronologie commune, publié comme capteur dérivé avec sa propre qualité | Montre que l'horodatage à la source sert à quelque chose |
| **Calibration en service** | Estimation continue de la dérive, coefficients appliqués à chaud, historique de calibration versionné | Différencie d'un simple driver |
| **Cartographie des bases de temps** | Trace de l'écart monotone ↔ horloge murale : tout abonné peut reconstruire une chronologie absolue commune | Argument technique fort, rarement traité |
| **Widget Screen temps réel** | Visualisation flux/qualité/latence directement sur la cible, via le framework Screen | Effet visuel, mais **Screen est indisponible en invité d'hyperviseur** (pas de virtio-gpu) |
| **Suite SAT et politique HAM amont** | Tests SAT prêts à l'emploi pour les objets publiés + politique HAM générique contribuable | Contribution à l'écosystème QNX |

### Palier 3 — après le 15 décembre (horizon 3 mois)

**`libglane`, l'API manquante** (bibliothèque publique et en-têtes documentés pour écrire son
propre publisher : c'est exactement le trou « aucune API », et la meilleure contribution amont
possible — à commencer dès que le socle est vert) · **Flux vers invité Linux** (transport en
mémoire partagée exposant les capteurs à un invité de l'hyperviseur : QNX capte, Linux affiche) ·
**Pont ROS 2 + GIF de README** (`ros2` est déjà porté sur QNX, donc la passerelle est crédible ; le GIF animé et l'installation QEMU en une commande servent le vote du public) ·
**Files adaptatives et contre-pression** · **Versionnement et migration des descriptions** ·
**Conteneur OCI QNX 8**.

---

## 4. Chemin critique daté

| Date | Jalon | Condition de passage |
|---|---|---|
| **12/11** | Licence QNX demandée + image Quick Start Raspberry Pi 5 téléchargée | Action humaine |
| **13/11** | **E2 tranché** : un processus tiers peut-il publier ses propres objets ? | Sinon bascule immédiate sur le repli : resmgr `/dev/sensors` + PPS |
| **14/11** | `CONTRACT.md` gelé ; `core/` desc + sample + calib + ring verts sur PC | ≥ 400 vérifications |
| **17/11** | Bus virtuel + scénarios + rejeu déterministe + jumeau Python + test différentiel | Palier 1 terminé |
| **21/11** | `gschema`, `glat`, `gplot` ; premiers chiffres de latence **sur Linux** | GLA-08, GLA-09 |
| **25/11** | Couche publication QNX + `glaned` sur QEMU ; **première preuve d'exécution sur QNX** | Nécessite le SDP |
| **28/11** | Deux publishers réels sur Raspberry Pi 5 ; HAM ; politique de sécurité ; fuzz et mutants | GLA-13, GLA-14, GLA-15 |
| **02/12** | Trace System Profiler, `glane doctor`, détection de capteur menteur, codelab | Palier 2 |
| **05/12** | **GEL FONCTIONNEL** | Plus aucune fonctionnalité |
| **12/12** | Vidéo 3 min, dépôt public, formulaire Devpost | Marge de 3 jours |

---

## 5. Ce qui maximise les prix

- **Catégorie Low-level Software** (4 000 $ + dauphin 2 000 $) : pilotes et composants construits
  avec le QNX Driver Development Kit. **Cette catégorie est vide dans ton portefeuille** —
  Passerelle est en Tooling, Ambre et Relais en Automotive.
- **Bonus « Best use of QNX-specific Technologies »** : la trace System Profiler, `ionotify`,
  HAM et la politique de sécurité sont exactement ce que le règlement cite.
- **Bonus Codelab** : `glane lab` auto-corrigé.
- **Vote Community Choice** : l'oscilloscope web embarqué et la démo chaos sont les deux actifs.

**Le point fort à marteler** : QNX livre le mécanisme et **un seul** publisher. Tu ne réinventes
pas le framework — tu fournis ce qui manque pour s'en servir (SDK, capteurs, simulation,
validation, mesure). C'est précisément ce que la documentation admet ne pas avoir.

---

## 6. Blocages, et qui les lève

| Blocage | Qui | Effort |
|---|---|---|
| Licence QNX + image Raspberry Pi 5 | **Toi** | 10 minutes + ~1 h d'attente |
| Essai E2 (publication par un processus tiers) | Un agent, lecture de doc + test | 2 h — **bloquant pour l'architecture** |
| Raspberry Pi 5 et un capteur I2C disponibles ? | **Toi** | sinon Palier 2 amputé du « duo réel/virtuel » |
| Frameworks GPIO/SPI réellement utilisables depuis un processus ordinaire (E3) | Un agent | 1 h |

**Repli prévu et écrit dans le cahier** : si E2 échoue, Glane devient un gestionnaire de
ressources `/dev/sensors` + PPS. Le projet reste valide en Low-level Software, il perd
seulement l'adossement au framework officiel.

---

## 7. Premier lot à lancer demain matin

**Lots L1 + L2 du cahier** (ils ne dépendent ni du SDP ni de E2) :

1. `core/` : `desc.c` (analyse et validation de la description JSON) + banque de **40 descriptions
   fautives**, chacune devant produire l'erreur attendue (champ, ligne, cause).
2. `core/` : `sample.c`, `calib.c` (arithmétique entière, polynôme degré ≤ 3), `ring.c`,
   `deadline.c` (péremption → `quality=STALE`, jamais de suppression silencieuse).
3. Tests en C11 `-std=c11 -O2 -Wall -Wextra -Werror`, binaires autonomes rendant 0,
   `OK <n> checks` en fin, plus une passe ASan/UBSan.

Critère de succès : `make test` vert sur Linux, ≥ 400 vérifications, garde de taille verte,
et **zéro** dépendance à QNX dans `core/`.
