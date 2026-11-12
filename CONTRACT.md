# Glane — contrat gelé

> **Ce fichier est le contrat.** Les en-têtes de `core/include/glane/` font foi
> sur les signatures ; ce document fige les invariants, les formats, et ce qui
> ne peut pas changer sans casser le jumeau Python et les abonnés. Les agents
> (sous-agents compris) **ne modifient jamais** ce contrat : ils l'implémentent.

## 0. Périmètre du contrat

Le socle portable `core/` ne connaît **ni QNX, ni horloge, ni fichier, ni
socket, ni `malloc`, ni état global mutable**. Tout ce qui touche au monde
réel est injecté par l'appelant via des pointeurs de fonction. Le socle
compile et se teste sur Linux avec :

```
gcc -std=c11 -O2 -Wall -Wextra -Werror
```

y compris sous ASan/UBSan.

## 1. ABI de la mesure — `glane_sample`

La mesure est un POD de **24 octets** (assertion de taille vérifiée en test),
sans pointeur, sans flottant.

| Champ          | Type       | Rôle                                                  |
|----------------|-----------|-------------------------------------------------------|
| `sensor_id`    | `uint32_t` | identifiant issu de la description                     |
| `seq`          | `uint32_t` | séquence monotone, **jamais réinitialisée**            |
| `ts_ns`        | `int64_t`  | horodatage à la source (`CLOCK_MONOTONIC`)            |
| `value_milli`  | `int32_t`  | valeur en **milli-unités** (jamais de `float`)        |
| `unit`         | `uint16_t` | `GLANE_UNIT_*`                                        |
| `quality`      | `uint8_t`  | `GLANE_Q_OK|STALE|SATURATED|FAULT|UNCERTAIN`          |
| `flags`        | `uint8_t`  | `GLANE_F_FIRST|CALIBRATED|INJECTED|SIMULATED`         |

**Invariants** :
- `quality` décrit la **mesure** ; `flags` décrit son **origine**. Ne jamais
  mélanger les deux (cela rendrait le rejeu ambigu).
- `seq` monotone : l'abonné détecte les pertes par discontinuité. Le retour
  à zéro après `UINT32_MAX` (wrap) n'est **pas** traité comme une perte.
- `ts_ns` est l'horodatage à la source, jamais l'heure de publication.

## 2. Calibration — arithmétique entière

Polynôme de degré `deg` (0..3) appliqué à `x_milli` (entrée brute, milli-unités).
Les coefficients sont **entiers, échelle `GLANE_CALIB_SCALE = 1 000 000`**
(donc `1.0` → `1000000`). L'accumulateur interne est **128 bits** (`__int128`,
extension de-facto supportée par gcc/clang sur x86-64 et aarch64 — donc par le
toolchain QNX). Aucun terme de degré 3 ne déborde dans les bornes validées
(voir §2.1).

**Convention d'unités** : le polynôme mappe `x_milli` → `value_milli`. Le terme
constant `a0` est donc en **milli-unités** (un décalage ajouté à la valeur en
milli) ; `a1` est un **gain sans dimension** ; `a2`, `a3` ont les dimensions
inverses correspondantes. Les kinds JSON acceptés sont **`none`** (identité) et
**`poly`** (polynôme complet, `coef = [a0, a1, ...]`). Un décalage seul s'écrit
`poly` avec `[offset_milli, 1.0]` ; un gain seul s'écrit `poly` avec `[0.0, gain]`.
Tout autre kind est refusé (`GLANE_DESC_ECALIB`). `calib.c` n'implémente donc
que `NONE` (identité) et `POLY` (Horner).

### 2.1 Bornes validées à l'analyse (`desc.c`)

Pour que le produit `|coef[3]| * |x|³` reste sous 2⁶² bits dans un int64
*avant* le passage en 128 bits :
- `|x_milli| <= 1 048 575` (≈ ±1048 unités, ample pour toute mesure réelle) ;
- `|coef[i]| <= 1 048 575` (soit |coefficient réel| ≤ 1.048575) ;
- `deg <= 3`.

Une description violant ces bornes est **refusée** par `glane_desc_validate`
avec `GLANE_DESC_ERANGE` et un message citant le champ. Le jumeau Python
refuse exactement les mêmes bornes.

### 2.2 Évaluation (Horner, int128)

```
acc = 0
for i = deg downto 0:
    acc = acc * x + coef[i]      /* tout en int128 */
result = round_half_away_from_zero(acc / GLANE_CALIB_SCALE)
```

`round_half_away_from_zero` : pour un quotient positif, `+SCALE/2` avant la
division entière ; pour un négatif, `-SCALE/2`. `NONE` = identité (retour
inchangé).

### 2.3 Jumeau Python (L3)

Le jumeau refait la **même** arithmétique entière avec des `int` Python
(précision arbitraire, donc toujours correct), sur les **mêmes** bornes.
Le test différentiel compare C et Python sur 10 000 valeurs et réduit
dichotomiquement le premier désaccord.

## 3. Description d'un capteur — JSON `sensors.d/*.json`

```json
{"id": 1, "name": "temp-chassis", "driver": "tmp",
 "bus": {"kind": "i2c", "dev": "/dev/i2c0", "addr": "0x48"},
 "period_ms": 100, "unit": "C", "range": [-40.0, 125.0],
 "calib": {"kind": "poly", "coef": [0.0, 1.0]},
 "thresholds": {"warn": 85.0, "crit": 105.0},
 "deadline_ms": 150, "stale_after_ms": 500}
```

**Champs obligatoires** : `id`, `name`, `driver`, `bus.kind`, `period_ms`,
`unit`, `range`, `deadline_ms`, `stale_after_ms`.
**Champs optionnels** : `calib` (défaut `NONE`), `thresholds`, `bus.dev`,
`bus.addr`.

`bus.kind` ∈ `i2c|spi|gpio|file|virtuel`. `range` est un tableau de 2 nombres
(min ≤ max). Les flottants sont parsés en **milli-entiers** avec arrondi au
plus proche (`125.0` → `125000`). `calib.kind` ∈ `none|poly` ;
`calib.coef` est un tableau de 1 à 4 nombres (longueur = deg+1).

**Toute description invalide est refusée avec un message précis** : champ,
numéro de ligne (1-based), cause. 40 descriptions fautives couvrent chaque
cas d'erreur attendu.

## 4. Tampon circulaire — `glane_ring`

Aucune allocation : le stockage est fourni par l'appelant. `push` écrase la
plus ancienne quand l'anneau est plein (et retourne 1). `find_gap` détecte
les pertes de `seq` (discontinuité consécutive, wrap non compté).

## 5. Péremption et panne — `glane_deadline`

- âge > `stale_after_ns` → `STALE` (jamais de suppression silencieuse) ;
- `fault_after` périodes manquantes consécutives → `FAULT`, transition
  journalisée par l'appelant.
- Une mesure fraîche remet le compteur de pannes à zéro (`alive`).

## 6. Schéma des objets publiés — `glane_schema`

Arborescence imposée : `/dev/sensors/<name>/{value,quality,seq,ts_ns,unit,info}`
plus `command/{period,calibrate,inject,selftest}`.

- Nom de capteur : non vide, ≤ 63 octets, charset `[a-z0-9._-]`, sans `.`
  initial ou final.
- Feuilles obligatoires : les 6 ci-dessus. `command` est optionnel.
- Le jumeau hôte `gschema` (Python) vérifie une arborescence réelle ; le C
  vérifie les noms avant publication. **Même contrat**.

## 7. Journal de mesures

Une ligne JSON par mesure, ASCII, entiers (les valeurs sont en milli-unités,
jamais de flottant dans le journal) :

```
{"s":"temp-chassis","id":1,"seq":4182,"ts_ns":18423390122,"d_ns":18424390122,"v_m":43125,"u":"C","q":"ok","f":"calibrated|simulated"}
```

- `ts_ns` : horodatage **à la source** (CLOCK_MONOTONIC).
- `d_ns` : horodatage de **livraison** à l'abonné. La latence d'une mesure est
  `d_ns - ts_ns` ; c'est ce que mesure `glat` (GLA-09). Champ optionnel : un
  journal sans `d_ns` reste valide, `glat` ignore alors ces lignes.
- `f` : drapeaux concaténés par `|` (`first`, `calibrated`, `injected`,
  `simulated`), ou `-` si aucun.
- `q` : qualité (`ok`, `stale`, `saturated`, `fault`, `uncertain`).

Le publisher virtuel (`tools/glaned-sim`) **rejoue un scénario de façon
déterministe** : deux exécutions avec les mêmes paramètres (description, forme
d'onde, graine, latence) donnent le même journal **octet pour octet** (GLA-04).

## 8. Ce qui est gelé, ce qui reste ouvert

**Gelé** (modifier = casser le jumeau et les abonnés) : l'ABI de `glane_sample`
(§1), l'échelle de calibration et ses bornes (§2), le format de description
(§3), le charset des noms publiés (§6), le format du journal (§7).

**Ouvert** (à figer après l'essai E2, §10 du cahier) : l'arborescence exacte
sous `/dev/sensors` (emplacements, types d'objets QNX), l'API de publication
côté QNX, l'intégration HAM.
