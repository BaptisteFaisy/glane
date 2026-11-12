# Glane

Couche capteurs et publication de périphériques pour **QNX 8.0**.

Projet candidat au **QNX Global Hackathon**, catégorie **Low-level Software**.
Échéance de soumission : 15 décembre 2026, 20 h (Europe/Paris).

## L'idée, en une phrase

Un capteur — réel sur Raspberry Pi 5 ou simulé sous QEMU — est décrit dans un
fichier, découvert, calibré, publié dans l'arborescence du système avec
horodatage à la source, et sa latence de publication est **mesurée et bornée**,
sans écrire un gestionnaire de ressources à la main.

QNX livre un framework de publication de périphériques et **un seul** publisher
(`usblauncher_otg`, orienté USB/stockage/média). Rien pour les capteurs, aucune
API de rédaction de publisher documentée, aucune simulation sans matériel, aucun
validateur de conformité, aucune mesure de latence. Glane fournit ce qui manque
pour s'en servir.

## Documents

- **Cahier des charges** : [`docs/GLANE-cahier-des-charges.md`](docs/GLANE-cahier-des-charges.md) (24 exigences GLA-01..24).
- **État et plan de poussée** : [`docs/GLANE-etat-et-poussee.md`](docs/GLANE-etat-et-poussee.md).
- **100 idées pour aller plus loin** : [`docs/GLANE-100-idees-aller-plus-loin.md`](docs/GLANE-100-idees-aller-plus-loin.md).
- **Matrice de traçabilité** : [`docs/GLANE-traceabilite.md`](docs/GLANE-traceabilite.md) (générée par `glane-trace`).
- **Contrat gelé** : [`CONTRACT.md`](CONTRACT.md) — ABI de la mesure, calibration entière, schéma publié, journal.

## Socle portable `core/`

C11 strict, pur (ni horloge, ni fichier, ni socket, ni `malloc`, ni état global),
compilable et testable sur Linux avec `gcc -std=c11 -O2 -Wall -Wextra -Werror`.

```
core/
├── include/glane/   glane_sample.h glane_calib.h glane_desc.h glane_bus.h
│                    glane_ring.h glane_deadline.h glane_schema.h
│                    glane_virtuel.h glane_saturate.h glane_selftest.h
│                    glane_jsonscan.h
├── src/             sample.c calib.c ring.c deadline.c schema.c
│                    desc.c desc_validate.c jsonscan.c
│                    virtuel.c saturate.c selftest.c
└── tests/           test_*.c + test_util.h + faulty/ (40 cas) + valid/
```

## Outils

| Outil | Langage | Rôle |
|---|---|---|
| `build/glaned-sim` | C (hôte) | publisher virtuel : desc → mesures synthétiques déterministes → journal |
| `build/calib_dump` | C (hôte) | dump de `glane_calib_apply` pour le test différentiel C↔Python |
| `host/gschema.py` | Python | validateur de conformité de l'arborescence publiée (GLA-08) |
| `host/glat.py` | Python | banc de latence p50/p99/p99.9 depuis un journal (GLA-09) |
| `host/greport.py` | Python | rapport HTML autonome d'une campagne de mesures |
| `host/gcanon.py` | Python | canonicalisation JSON + empreinte SHA-256 stable |
| `host/glane_twin.py` | Python | jumeau de la calibration entière (test différentiel, GLA-05) |
| `scripts/glane_mutate.py` | Python | test de mutation du socle C (≥ 8 mutants, tous tués) |
| `scripts/glane_trace.py` | Python | matrice de traçabilité GLA-01..24 |
| `scripts/sizecheck.py` | Python | garde de taille (< 500 lignes/fichier, < 50/fonction) |

## Construire et tester

```sh
make            # bibliothèque statique + binaires de test + outils hôte
make test       # tests C — verdict "OK <n> checks" par binaire
make host-test  # tests hôte Python (unittest) : jumeau, différentiel, gschema, glat, greport, gcanon, trace
make asan       # tests C sous AddressSanitizer + UndefinedBehaviorSanitizer
make sizecheck  # garde de taille
python3 scripts/glane_mutate.py   # test de mutation (tous les mutants doivent être tués)
python3 scripts/glane_trace.py     # matrice de traçabilité (Markdown)
```

### Démo express (publisher virtuel → latence → rapport)

```sh
make all
./build/glaned-sim core/tests/valid/temp-chassis.json 2000 sine 42 1500000 /tmp/j.jsonl
python3 host/glat.py /tmp/j.jsonl                 # p50/p99/p99.9 de latence
python3 host/greport.py /tmp/j.jsonl /tmp/rapport.html
```

Deux exécutions de `glaned-sim` avec les mêmes paramètres produisent le même
journal **octet pour octet** (rejeu déterministe, GLA-04).

## État des vérifications

- Tests C : **973 vérifications** (sample, calib, ring, deadline, schema, desc, virtuel, saturate, selftest), ASan/UBSan vert.
- Test différentiel C↔Python : **10 000 valeurs** de calibration, graine fixe, zéro divergence.
- Test de mutation : **10/10 mutants tués**.
- Banque de descriptions fautives : **40 cas**, chacun produisant le code d'erreur attendu.
- Garde de taille : verte (< 500 lignes/fichier, < 50 lignes/fonction).

## Licence

Apache-2.0 — voir [`LICENSE`](LICENSE).