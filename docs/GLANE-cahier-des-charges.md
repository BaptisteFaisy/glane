# Glane — couche capteurs et publication de périphériques pour QNX 8.0

**Nom de travail : « Glane »** (récolter ce que les capteurs donnent, sans rien perdre).
Version 0.1 — 11 novembre 2026 — projet candidat au QNX Global Hackathon, catégorie **Low-level Software**.
Classement interne : 1er sur 140 idées notées (15/16), et 1er aussi quand on retire le critère « sans SDP ».
Deux calendriers possibles : **5 semaines** (soumission au concours, deadline 15/12/2026 à 20 h de Paris) ou **3 mois** (produit complet).

---

## 0. Lecture rapide

**Ce que c'est.** Une couche d'abstraction capteurs pour QNX 8.0 : au-dessus des frameworks GPIO et SPI déjà fournis, elle offre (a) une bibliothèque portable de description, découverte et calibration de capteurs, (b) un SDK pour écrire un *publisher* de périphériques, (c) cinq publishers de référence, (d) un **publisher virtuel** qui injecte des mesures synthétiques sous QEMU sans aucun matériel, (e) un validateur de schéma des objets publiés, (f) un banc de mesure de la latence de publication.

**La promesse, en une phrase.** Un capteur — réel sur Raspberry Pi 5 ou simulé sous QEMU — est décrit dans un fichier, découvert, calibré, publié dans l'arborescence du système avec horodatage à la source, et sa latence de publication est **mesurée et bornée**, sans écrire un gestionnaire de ressources à la main.

**Trois idées qui portent le projet.**

1. **Le framework existe, l'écosystème n'existe pas.** QNX livre un mécanisme de publication de périphériques et **un seul** publisher (`usblauncher_otg`, orienté USB/stockage/média). Rien pour les capteurs. On ne réinvente pas le mécanisme : on fournit ce qui manque pour s'en servir.
2. **Tout doit être simulable.** Aucune simulation sans matériel n'est documentée dans le guide. Le publisher virtuel devient donc à la fois l'outil de démonstration, le harnais de test en CI, et la raison pour laquelle le projet est vérifiable **avant** d'avoir une carte.
3. **Mesurer plutôt qu'affirmer.** Rien n'est dit sur la latence de publication ni sur l'horodatage des notifications. Le banc horodaté (attachement → objet publié → abonné servi) transforme une affirmation en chiffre.

**Trois niveaux d'ambition.** Chaque exigence porte un niveau : **S** (socle, indispensable à une soumission), **C** (complet, la version visée), **X** (extrême, ce qui pousse l'idée au bout).

**À valider avant d'écrire la moindre ligne de produit** (section 10) : quatre essais de faisabilité. Le plus critique vérifie qu'un processus tiers peut créer et publier ses propres objets dans l'arborescence du service de publication, sans code propriétaire ni licence commerciale.

**Statut des affirmations.** La section 14 sépare ce qui est vérifié dans la documentation de ce qui reste une hypothèse de conception.

---

## 1. Contexte et cas d'usage

### 1.1 Qui a ce problème

| Acteur | Besoin | Pourquoi QNX |
|---|---|---|
| Fabricant de dispositifs médicaux | Remonter température, pression, débit, position, batterie d'un appareil certifiable, avec horodatage fiable et traçable | Déterminisme + cloisonnement par politique de sécurité (IEC 62304) |
| Équipementier automobile / ECU | Capteurs de batterie, thermique, courant, position sur une plateforme zonale | QNX équipe déjà le parc ; les capteurs arrivent par I2C/SPI/CAN |
| Intégrateur industriel | Supervision de capteurs d'une machine (vibration, courant, température) pour la maintenance prédictive | Cadence garantie et isolation des bus lents |
| Roboticien | Fusion de mesures hétérogènes à cadences différentes, sans perte ni réordonnancement | Passage de messages synchrone, priorités |
| Enseignement / prototypage | Raspberry Pi 5 + QNX 8.0 : image Quick Start officielle, capteurs à 5 € | Cible explicitement supportée par le concours |

### 1.2 Pourquoi maintenant

- QNX 8.0 est gratuit en usage non commercial (QNX Everywhere, licence obtenue en ~1 h).
- Une image Quick Start pour Raspberry Pi 5 est publiée officiellement : le matériel du concours est le matériel réel du projet.
- Le framework de publication de périphériques est documenté mais **presque vide** : un seul publisher livré, aucune API de rédaction documentée, aucun exemple de capteur. C'est le genre de trou qu'un concours existe pour combler.

### 1.3 Ce que le projet n'est pas

Ni un framework de fusion de capteurs, ni un SLAM, ni un bus de terrain. Glane s'arrête à la publication fiable et mesurée d'une mesure horodatée.

---

## 2. Ancrage documentaire

### 2.1 Ce que QNX fournit déjà (à ne pas réinventer)

| Élément | Source |
|---|---|
| Framework de publication de périphériques, objets publiés dans une arborescence à **emplacements fixes**, modèle de données orienté média/stockage (mountpoint, partitions, TOC, température et mécanisme du disque) | `com.qnx.doc.dev_pub/topic/about.html`, `overview.html`, `rmps_objects.html` |
| **Un** publisher livré : `usblauncher_otg` (USB, stockage, média), montage automatique via `rules.mnt` | `dev_pub/usblauncher_otg.html` |
| Mécanisme de plugins (generic, ipod, mediafs, audiocd, dvd) avec manager et système de *ratings* auto-déclarés | `dev_pub/plugins.html` |
| Framework **GPIO** documenté | `com.qnx.doc.gpio_framework/topic/overview.html` |
| Framework **SPI** documenté (technote) | `com.qnx.doc.neutrino.spi_framework/topic/about.html` |
| Exemple officiel `rpi-gpio` (gestionnaire de ressources GPIO + modules Python) | `gitlab.com/qnx/projects` |
| `libusb` porté ; `libcamera` porté | index `github.com/qnx-ports/build-files/ports` |
| HAM, PPS, `ionotify`, `TimerPulse`, politiques de sécurité | System Architecture, System Security Guide |

### 2.2 Les trous exploités (tous sourcés)

| Code | Trou | Source |
|---|---|---|
| PUB-SDK | Aucune API ni SDK « écrire un publisher » : le guide ne décrit que le contenu des objets et les options en ligne de commande | `dev_pub/about.html` (lu) |
| PUB-SENS | Un seul publisher livré ; rien pour capteurs, I2C/SPI, BLE, batterie, thermique | `dev_pub/overview.html` (lu) |
| PUB-SIM | Aucune simulation ni test sans matériel mentionné dans tout le guide ; pas de publisher virtuel pour QEMU | `dev_pub/about.html` (lu, sommaire complet) |
| PUB-CONF | Aucun validateur de conformité des objets publiés, alors que l'arborescence à emplacements fixes est imposée | `dev_pub/rmps_objects.html` (lu) |
| PUB-LAT | Rien sur la latence de publication, l'horodatage des notifications, la qualité de service | `dev_pub/rmps_objects.html` (lu) |
| PUB-META | Modèle de données sans unités, plages, calibration ni horodatage de capteur | `dev_pub/rmps_objects.html` (lu) |
| PUB-LIFE | Publishers décrits comme processus persistants redémarrables, mais aucune intégration HAM/cycle de vie documentée | `dev_pub/overview.html` (lu) |
| PUB-SEC | Aucune permission ni politique de sécurité mentionnée pour les objets publiés, y compris l'objet de commande | `dev_pub/rmps_objects.html` (lu) |
| SENS-ABS | GPIO et SPI ont des frameworks, mais aucune abstraction capteur au-dessus (fusion, calibration, découverte) | `references.html` (lu) |
| HID-DOC | Aucune documentation HID/entrée dans le nav SDP | `guides.html`, `references.html` (lus) |

---

## 3. Périmètre

### 3.1 Dans le périmètre

- Capteurs **numériques** à lecture périodique sur I2C, SPI, GPIO (compteur/impulsion) et sur fichier (thermique sysfs-like).
- Description déclarative d'un capteur : type, bus, adresse, période, unité, plage, calibration (offset/gain/polynôme), seuils.
- Découverte, calibration, horodatage à la source, publication, notification aux abonnés.
- Publisher **virtuel** (mesures synthétiques scriptables : sinusoïde, rampe, bruit, panne, dérive, saturation).
- Validateur de schéma des objets publiés + banc de latence.
- Cinq publishers de référence : température, IMU 6 axes, courant/tension, batterie, encodeur/impulsion.
- Politique de sécurité type pour l'ensemble (objet de commande protégé).

### 3.2 Hors périmètre (assumé)

Capteurs analogiques nécessitant un CAN externe non documenté ; BLE (pile absente du périmètre vérifié) ; fusion multi-capteurs et filtrage avancé (Kalman) ; caméra et `libcamera` ; audio ; tout ce qui exige du matériel non disponible sur Raspberry Pi 5 ou QEMU.

---

## 4. Architecture

```
glane/
├── CONTRACT.md              # contrat gelé : en-têtes et formats, non modifiables par les agents
├── core/                    # C11 portable, PUR : ni horloge, ni fichier, ni socket, ni malloc, ni état global
│   ├── include/glane/       #   glane_desc.h glane_sample.h glane_calib.h glane_schema.h glane_bus.h
│   ├── src/                 #   desc.c sample.c calib.c schema.c ring.c deadline.c
│   └── tests/               #   test_desc.c test_calib.c test_schema.c test_ring.c test_util.c
├── bus/                     # adaptateurs de bus : i2c.c spi.c gpio.c file.c virtuel.c (+ stub PC)
├── sensors/                 # drivers de capteurs : tmp.c imu.c ina.c batt.c encoder.c
├── publish/                 # couche publication : objet, arborescence, notification, commande
│   └── qnx/                 # glue QNX isolée (#ifdef __QNX__), jamais compilée sur PC
├── target/                  # binaires QNX : glaned (publisher), glanectl (CLI), glane-sim
├── host/                    # outils hôte Python 3 stdlib : gschema (validateur), glat (banc latence),
│                            # gsim (générateur de scénarios), gplot (rapport HTML)
├── image/                   # fragment de buildfile d'image + politique de sécurité type
├── scenarios/               # *.json : scénarios de mesures et de pannes injectées
├── tests/                   # différentiels, fuzz du parseur, banque de descriptions fautives
├── codelab/                 # 4 chapitres : « publier un capteur sous QNX 8.0 »
└── docs/                    # spec-fr.md, spec-en.md, ARCHITECTURE.md, DECISIONS.md, VALIDATION.md
```

**Règle de portabilité** (identique à `relais`) : `core/` ne connaît ni QNX ni le temps. L'horloge, les fichiers et les sockets sont injectés par l'appelant via des pointeurs de fonction. Tout `core/` compile et se teste sur Linux avec `gcc -std=c11 -O2 -Wall -Wextra -Werror`, y compris sous ASan/UBSan.

---

## 5. Contrat

### 5.1 ABI de la mesure (authoritative : `core/include/glane/glane_sample.h`)

```c
typedef struct {
  uint32_t sensor_id;     /* identifiant issu de la description          */
  uint32_t seq;           /* numéro de séquence monotone, jamais réinitialisé */
  int64_t  ts_ns;         /* horodatage à la source, CLOCK_MONOTONIC     */
  int32_t  value_milli;   /* valeur en milli-unités (échelle entière)    */
  uint16_t unit;          /* GLANE_UNIT_* : C, K, V, mA, deg_s, g, rpm…  */
  uint8_t  quality;       /* GLANE_Q_OK|STALE|SATURATED|FAULT|UNCERTAIN  */
  uint8_t  flags;         /* GLANE_F_FIRST|CALIBRATED|INJECTED|SIMULATED */
} glane_sample;           /* 24 octets, alignement naturel, aucun pointeur */
```

Choix à ne pas perdre : **échelle entière en milli-unités** (pas de `float`) pour que le jumeau de vérification en Python rejoue bit à bit ; `seq` monotone pour détecter pertes et réordonnancements ; `quality` distinct de `flags` (la qualité décrit la mesure, les drapeaux décrivent son origine).

### 5.2 Description d'un capteur (`sensors.d/*.json`, valide aussi pour le simulateur)

```json
{"id": 1, "name": "temp-chassis", "driver": "tmp", "bus": {"kind": "i2c", "dev": "/dev/i2c0", "addr": "0x48"},
 "period_ms": 100, "unit": "C", "range": [-40.0, 125.0],
 "calib": {"kind": "poly", "coef": [0.0, 1.0]},
 "thresholds": {"warn": 85.0, "crit": 105.0},
 "deadline_ms": 150, "stale_after_ms": 500}
```

`kind` ∈ `i2c|spi|gpio|file|virtuel`. `deadline_ms` = borne au-delà de laquelle la mesure est publiée en `STALE` au lieu d'être jetée.

### 5.3 Arborescence publiée (à figer après l'essai E2)

```
/dev/sensors/<name>/{value,quality,seq,ts_ns,unit,info}
/dev/sensors/<name>/command/{period,calibrate,inject,selftest}
```

### 5.4 Journal de mesures (une ligne JSON par mesure, ASCII, flottants `%.6g`)

```
{"s":"temp-chassis","id":1,"seq":4182,"ts_ns":18423390122,"v_m":43125,"u":"C","q":"ok","f":"calibrated"}
```

---

## 6. Exigences

| ID | Niveau | Exigence | Vérification |
|---|---|---|---|
| GLA-01 | S | `core/` compile sur Linux en C11 strict sans avertissement et sans dépendre de QNX | `make core` + garde de compilation croisée |
| GLA-02 | S | Une description JSON est analysée et validée ; toute description invalide est refusée avec un message précis (champ, ligne, cause) | `test_desc` + banque de 40 descriptions fautives |
| GLA-03 | S | Un capteur virtuel produit des mesures à la période déclarée, avec `seq` monotone et horodatage à la source | `test_ring`, `test_deadline` |
| GLA-04 | S | Le publisher virtuel rejoue un scénario de fichier de manière déterministe : deux exécutions donnent le même journal octet pour octet | test de rejeu avec graine fixe |
| GLA-05 | S | La calibration (offset, gain, polynôme degré ≤ 3) est appliquée dans `core/` en arithmétique entière, identique au jumeau Python | test différentiel C ↔ Python sur 10 000 valeurs |
| GLA-06 | S | Une mesure plus vieille que `stale_after_ms` est publiée avec `quality=STALE`, jamais jetée silencieusement | `test_deadline` |
| GLA-07 | S | Un capteur qui ne répond plus est signalé `FAULT` après N périodes manquantes, et le passage est journalisé | injection de panne dans le scénario |
| GLA-08 | S | Le validateur de schéma hôte (`gschema`) vérifie l'arborescence publiée et rend un code de retour non nul en cas d'écart | test sur arborescence de référence + 12 variantes cassées |
| GLA-09 | S | Le banc de latence (`glat`) mesure attachement → publication → abonné, et produit p50/p99/p99.9/max en JSON et en HTML | exécution sur le publisher virtuel |
| GLA-10 | S | La CI tient en moins de 5 minutes par job, `fetch-depth: 1`, `concurrency` posé | garde `ci-budget` |
| GLA-11 | S | Tout fichier fait moins de 500 lignes, toute fonction moins de 50 lignes, aucune duplication de logique | `scripts/sizecheck.py` |
| GLA-12 | C | Un publisher réel tourne sur QNX 8.0 (Raspberry Pi 5 ou QEMU) et publie dans `/dev/sensors` | essai E2 puis E4, capture vidéo |
| GLA-13 | C | Au moins deux capteurs réels sur Raspberry Pi 5 (thermique + un capteur I2C à 5 €) | vidéo, journal horodaté |
| GLA-14 | C | Les publishers sont supervisés par HAM : un publisher tué est relancé et la reprise est mesurée | essai de reprise, chiffre publié |
| GLA-15 | C | Une politique de sécurité type est fournie et vérifiée : l'objet `command` n'est accessible qu'aux types autorisés | `secpolcompile` + test de refus |
| GLA-16 | C | Notification aux abonnés sans perte sous charge : aucun `seq` manquant sur 60 s à 1 kHz cumulé | test de charge, compteur de pertes |
| GLA-17 | C | Le parseur de description est fuzzé (entrées déterministes par graine) sans crash ni fuite | fuzz ASan, 100 000 entrées |
| GLA-18 | C | Un codelab de 4 chapitres permet à un débutant de publier un capteur virtuel puis réel | relecture à froid par un agent neuf |
| GLA-19 | X | Découverte automatique : balayage I2C/SPI, identification par registre `WHO_AM_I`, proposition de description | démo de découverte |
| GLA-20 | X | Qualité de service : budget de publication par capteur, dégradation declared quand le bus sature | mesure sous saturation de bus |
| GLA-21 | X | Horodatage matériel quand le bus le permet, et preuve de l'écart avec l'horodatage logiciel | essai comparatif |
| GLA-22 | X | Publisher générique piloté par script (aucun C à écrire pour un capteur simple) | démo |
| GLA-23 | X | Export des mesures vers un abonné PPS et vers un fichier circulaire borné | test d'export |
| GLA-24 | X | Contribution amont : proposition d'un chapitre « écrire un publisher » ou d'un exemple capteur | URL de la demande de fusion |

---

## 7. Plan de vérification

1. **Unitaires C** (`core/tests/`) : binaires autonomes, retour 0 si vert, une ligne par vérification, `OK <n> checks` à la fin. Cible : ≥ 600 vérifications.
2. **Différentiel C ↔ Python** : la calibration, le séquencement et la détection de péremption existent en deux implémentations indépendantes ; un test par graine compare sur 10 000 valeurs et **réduit dichotomiquement** le premier désaccord (méthode éprouvée sur Passerelle).
3. **Banque de cas fautifs** : 40 descriptions invalides, 12 arborescences cassées, 8 scénarios de panne — chaque cas doit produire l'erreur attendue, pas seulement « une erreur ».
4. **Fuzz déterministe** du parseur sous ASan/UBSan.
5. **Mutants** : au moins 8 mutations du code de décision (seuils, ordre de publication, gestion de `seq`) ; chacune doit être tuée par au moins un test.
6. **Banc de latence** : mesures réelles publiées dans le rapport, aucune cible non chiffrée.
7. **Gardes maison** : `sizecheck.py` (< 500 lignes/fichier, < 50 lignes/fonction), `spdx-guard` (en-tête de licence sur chaque fichier), `ci-budget` (≤ 5 min/job).

---

## 8. CI

- `timeout-minutes: 5` par job, `concurrency` avec annulation des runs obsolètes, `actions/checkout@v4` en `fetch-depth: 1`.
- Deux jobs parallèles : `build-test` (core + bus + sensors, ASan/UBSan) et `host-test` (pytest, validateur, banc de latence sur scénarios).
- Aucun job ne construit pour QNX en CI tant que le SDP n'est pas disponible sur un runner ; dès qu'il l'est, un job `qnx-build` optionnel, hors PR, sur événement manuel.
- Runners self-hosted existants réutilisables ; garde `ci-budget` obligatoire.

---

## 9. Jalons

### 9.1 Option 5 semaines (soumission au concours)

| Semaine | Livrable |
|---|---|
| S1 | Essais E1–E4 (section 10) tranchés ; `CONTRACT.md` gelé ; `core/` desc+sample+calib+ring avec tests verts sur PC |
| S2 | Bus virtuel + scénarios + rejeu déterministe ; jumeau Python ; tests différentiels ; validateur de schéma |
| S3 | Couche publication QNX (si le SDP est installé) ; `glaned` ; banc de latence ; premiers chiffres |
| S4 | Deux publishers réels sur Raspberry Pi 5 ; HAM ; politique de sécurité ; fuzz et mutants |
| S5 | Codelab, docs EN/FR, vidéo 3 min, dépôt public, formulaire Devpost, marge |

**Si le SDP n'est pas installé en S3** : livrer le socle portable + le publisher virtuel + le banc de latence sur Linux, et l'annoncer explicitement dans la soumission. C'est une soumission plus faible mais honnête ; la section 13.4 dit comment la formuler.

### 9.2 Option 3 mois

M1 = socle portable complet et prouvé (S1–S2 + fuzz + mutants). M2 = cible QNX, publishers réels, HAM, politique de sécurité, mesure de latence sur carte. M3 = découverte automatique, QoS, export PPS, codelab étendu, contribution amont, dossier de validation.

---

## 10. Essais de faisabilité (semaine 1, avant tout code produit)

| ID | Question | Comment trancher | Bloquant si |
|---|---|---|---|
| E1 | Le SDP 8.0 s'installe-t-il et QEMU démarre-t-il une image QNX ? | Compte myQNX → formulaire QNX Everywhere → licence (~1 h annoncée) → QNX Software Center → image Quick Start | non (repli : socle portable seul) |
| E2 | Un processus tiers peut-il créer et publier ses propres objets dans l'arborescence du service de publication ? | Lire `dev_pub/overview.html` + `rmps_objects.html` en détail ; au besoin tester avec le publisher existant ; poser la question sur le Discord QNX et r/qnx | **oui** : basculer sur PPS ou sur un gestionnaire de ressources `/dev/sensors` maison |
| E3 | Les frameworks GPIO et SPI sont-ils utilisables depuis un processus ordinaire sur Raspberry Pi 5 ? | Lire `gpio_framework/overview.html` et la technote SPI ; croiser avec l'exemple officiel `rpi-gpio` | oui pour GLA-13 : se limiter au capteur thermique et au virtuel |
| E4 | L'horodatage à la source est-il fiable à la milliseconde sur I2C/SPI ? | Mesurer l'écart entre lecture du bus et `ClockTime(CLOCK_MONOTONIC)` sur 10 000 échantillons | non : publier l'écart mesuré au lieu de promettre une borne |

---

## 11. Risques

| Risque | Probabilité | Impact | Parade |
|---|---|---|---|
| E2 échoue : publication impossible depuis un processus tiers | moyenne | fort | Repli annoncé : gestionnaire de ressources `/dev/sensors` + PPS, ce qui reste un projet Low-level valide |
| Le SDP tarde (compte, licence, téléchargement) | moyenne | moyen | Tout le socle est testable sur PC ; la cible n'arrive qu'en S3 |
| Le modèle de données orienté média ne se prête pas aux capteurs | faible | moyen | GLA-08 : publier un schéma étendu et documenter l'écart (c'est précisément le trou PUB-META) |
| Capteurs réels instables sur Pi 5 (bus, alimentation) | moyenne | faible | Le publisher virtuel reste la démonstration principale ; le réel est un bonus filmé |
| Un projet officiel QNX couvre déjà les capteurs (livre non lu) | faible | fort | Vérifier `guides.html` et `references.html` avant S2 ; chercher « sensor » sur oss.qnx.com |

---

## 12. Script vidéo (3 minutes, sans musique sous droits, sans marque tierce)

| Temps | Plan |
|---|---|
| 0:00–0:20 | Écran : un seul publisher existe dans QNX. Énoncé du trou, citations de la doc à l'appui |
| 0:20–0:50 | Terminal hôte : `gschema` valide une arborescence, `glat` sort p50/p99/p99.9 |
| 0:50–1:40 | QEMU : `glaned` démarre, 5 capteurs virtuels publiés, scénario de panne injecté → `STALE` puis `FAULT`, horodatage visible |
| 1:40–2:25 | Raspberry Pi 5 sous QNX 8.0 : capteur thermique réel, valeurs lues, journal, puis capteur I2C |
| 2:25–2:50 | Preuves : tests verts (nombre de vérifications), test différentiel C ↔ Python, mutants tués, garde de taille |
| 2:50–3:00 | Rappel : ce que QNX apporte (cadence, priorités, horodatage, HAM, politique de sécurité) |

---

## 13. Dossier de soumission Devpost

1. **Dépôt public** créé pendant la période de soumission, licence ouverte détectable en tête de page (Apache-2.0 recommandé).
2. **Catégorie** : Low-level Software.
3. **Preuve d'usage de QNX** (obligatoire) : pointer nommément `publish/qnx/*.c`, `target/glaned.c`, le fragment de buildfile dans `image/`, la politique de sécurité, et les captures d'écran/vidéo sur QNX 8.0 avec la version du SDP et la cible.
4. **Texte de description** : problème (un seul publisher livré, aucune API, aucune simulation, aucune mesure de latence) → contribution (SDK, 5 publishers, simulateur, validateur, banc) → preuve (chiffres de latence, nombre de tests, mutants) → reproductibilité (versions, cible, toolchain).
5. **Instructions de build et d'exécution** : version QNX, cible (QEMU et/ou Raspberry Pi 5), toolchain, commandes exactes.
6. **Bonus Codelab** : renseigner l'URL du codelab (4 chapitres) dans le champ prévu.
7. **Langue** : tout en anglais, ou traduction anglaise fournie (règlement).

---

## 14. Statut des affirmations

**Vérifié dans la documentation** (pages lues) : existence du framework de publication et de ses objets à emplacements fixes ; un seul publisher livré ; mécanisme de plugins ; absence d'API de rédaction de publisher ; absence de simulation sans matériel ; absence de validateur de conformité ; absence de toute information de latence/horodatage ; modèle de données orienté média ; frameworks GPIO et SPI documentés ; `rpi-gpio` officiel ; `libusb` et `libcamera` portés.

**Hypothèses de conception à confirmer** : qu'un processus tiers puisse publier ses propres objets (E2) ; la forme exacte de l'arborescence publiée (à figer après E2) ; la disponibilité effective de l'I2C sur l'image Quick Start Raspberry Pi 5 ; le comportement de HAM avec un publisher ; l'absence d'un framework capteurs dans un livre non lu (Device Publishers lu, mais Screen, audio, Technotes et SMMUMAN ne le sont pas).

**Volontairement non affirmé** : aucun chiffre de latence n'est annoncé avant mesure. Toute borne publiée dans la soumission doit sortir du banc `glat`.

---

## 15. Règles de code et délégation

**Règles maison (non négociables)** : C11, `-std=c11 -O2 -Wall -Wextra -Werror` ; `core/` pur (ni horloge, ni fichier, ni socket, ni allocation, ni état global mutable) ; code spécifique à QNX confiné dans `publish/qnx/` et `target/`, gardé par `#ifdef __QNX__` et jamais cassant pour le build PC ; moins de 500 lignes par fichier, moins de 50 lignes par fonction ; aucune duplication (un seul implémentation par logique, helpers partagés dans `core/src/`) ; Python 3 standard uniquement ; chaque exigence satisfaite est citée en commentaire au-dessus du code et dans le nom du test correspondant.

**Découpage en lots pour sous-agents** (un lot = un agent, brief complet et autonome) :

| Lot | Contenu | Dépend de |
|---|---|---|
| L1 | `core/` desc + schema + tests + banque de cas fautifs | CONTRAT gelé |
| L2 | `core/` sample + calib + ring + deadline + tests | L1 |
| L3 | Jumeau Python (calibration, séquence, péremption) + test différentiel + réduction dichotomique | L2 |
| L4 | `bus/virtuel.c` + `scenarios/` + rejeu déterministe | L2 |
| L5 | `host/gschema` + `host/glat` + `host/gplot` | L4 |
| L6 | Fuzz du parseur + mutants + gardes (sizecheck, spdx, ci-budget) | L1–L5 |
| L7 | `publish/qnx/` + `target/glaned` + `image/` (nécessite le SDP) | E1, E2 |
| L8 | Drivers réels + Raspberry Pi 5 + HAM (nécessite le SDP et la carte) | L7, E3 |
| L9 | Codelab 4 chapitres + docs EN/FR + script vidéo | L1–L6 |

Chaque brief de sous-agent doit rappeler : contexte complet (l'agent ne partage aucune mémoire), critère de succès mesurable, bornes (temps, nombre d'essais, comportement en cas d'échec), règles shell (jamais `node -e`, `timeout` devant les commandes longues), et « fais-le toi-même, sans déléguer ».
