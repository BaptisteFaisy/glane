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

## État

- **Cahier des charges** : [`docs/GLANE-cahier-des-charges.md`](docs/GLANE-cahier-des-charges.md)
  (24 exigences GLA-01 à GLA-24, 4 essais de faisabilité, 9 lots délégables).
- **État et plan de poussée** : [`docs/GLANE-etat-et-poussee.md`](docs/GLANE-etat-et-poussee.md).
- **Contrat gelé** : [`CONTRACT.md`](CONTRACT.md) — l'ABI de la mesure, la
  calibration entière, le format de description, le schéma publié.

## Socle portable `core/`

C11 strict, pur (ni horloge, ni fichier, ni socket, ni `malloc`, ni état global),
compilable et testable sur Linux avec :

```
gcc -std=c11 -O2 -Wall -Wextra -Werror
```

```
core/
├── include/glane/   glane_sample.h glane_calib.h glane_desc.h glane_bus.h
│                    glane_ring.h glane_deadline.h glane_schema.h
├── src/             sample.c calib.c (desc.c ring.c deadline.c schema.c à venir)
└── tests/           test_*.c + test_util.h + faulty/ (banque de cas fautifs)
```

### Construire et tester

```sh
make            # bibliothèque statique + binaires de test
make test       # exécute tous les tests, verdict "OK <n> checks" par binaire
make asan       # même chose sous AddressSanitizer + UndefinedBehaviorSanitizer
make sizecheck  # garde : < 500 lignes/fichier, < 50 lignes/fonction
```

Chaque binaire de test rend 0 et imprime `OK <n> checks` en fin de parcours.

## Licence

Apache-2.0 — voir [`LICENSE`](LICENSE).