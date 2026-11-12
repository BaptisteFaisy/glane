# Glane — 100 idées pour aller plus loin

> Générées le 2026-11-12. Complément du cahier des charges (24 exigences GLA-01..24)
> et des 25 leviers de `qnx-100-aller-plus-loin.md` (lot 2). **Aucun doublon** avec
> ces deux listes : ce sont 100 idées *neuves*, classées en 4 lots de 25.
>
> Format : `N. **Nom** — direction | ce que ça ajoute | effort | risque | gain 1-5`
> Directions : **a** profondeur technique · **b** utilité produit · **c** écosystème
> QNX/amont · **d** gain concours · **e** valeur au-delà du concours.
>
> Les **10 meilleures** (gratuites, réalisables sur Linux sans SDP ni matériel,
> forte valeur) sont marquées ⭐ et implémentées — voir §5.

---

## Lot A — Profondeur technique du socle `core/` (C11 pur)

1. **Arithmétique saturante contractuelle** ⭐ — a+b+d | Formalise les dépassements `int64` et transforme une saturation en `SATURATED` traçable sans wraparound, avec vecteurs aux limites. | effort moyen | risque moyen | gain 5
2. **Propagation d'incertitude d'étalonnage** — a+b+e | Propage une borne d'erreur entière à travers offset, gain et polynôme, puis signale `UNCERTAIN` quand l'incertitude dépasse le budget admis. | effort élevé | risque élevé | gain 5
3. **Domaine de validité de l'étalonnage** — a+b | Refuse l'extrapolation hors du domaine déclaré et conserve le dernier jeu de coefficients valide en marquant `FAULT`. | effort moyen | risque moyen | gain 4
4. **Garde-fou polynomial avant multiplication** — a+d | Évalue le polynôme par Horner avec saturation bornée à chaque étape et signale le dépassement avant écriture. | effort moyen | risque moyen | gain 4
5. **Activation transactionnelle des coefficients** — a+b | Valide un jeu de coefficients puis l'active par échange atomique, avec restauration de la version précédente en cas d'échec. | effort moyen | risque moyen | gain 4
6. **ABI opaque et versionnée du core** — a+c+e | Poignées opaques, tailles de structure interrogables, version ABI et codes d'erreur indépendants d'`errno`. | effort élevé | risque moyen | gain 5
7. **Index de routage statique sans collision** — a+b+d | Index trié `sensor_id→slot` en mémoire statique, détection de doublons, dispatch logarithmique sans allocation. | effort moyen | risque moyen | gain 4
8. **Parseur de description incrémental à budget fixe** — a+c+e | Accepte la description JSON par fragments dans un tampon borné, rejette doublons et UTF-8 invalide, sans arbre alloué. | effort élevé | risque moyen | gain 5
9. **Table de description générée au build** — a+c+d | Transforme chaque description validée en table C constante à la compilation, supprimant le parseur du chemin rapide. | effort élevé | risque moyen | gain 5
10. **Propriétés métamorphiques de la description** — a+d+e | Prouve que réordonner les clés ou changer les espaces ne change ni la table de routage ni l'empreinte canonique. | effort moyen | risque faible | gain 4
11. **Grammaire d'unités à conversion entière exacte** — a+b+e | Formalise dimensions et facteurs rationnels des unités, convertit sans flottant avec détection de dépassement. | effort élevé | risque moyen | gain 4
12. **Automate de qualité vérifié exhaustivement** ⭐ — a+d+e | Énumère les suites courtes d'événements (fraîcheur, défaut, saturation) et compare bit à bit les transitions C avec un modèle de référence. | effort élevé | risque faible | gain 5
13. **Époque de session dans le registre de mesure** — a+b | Ajoute une époque de redémarrage hors du POD 24 octets pour distinguer une reprise du publisher d'une vraie perte de `seq`. | effort moyen | risque moyen | gain 4
14. **Horodatage avec intervalle d'incertitude** — a+b+e | Associe une incertitude temporelle à chaque source, calcule l'âge comme intervalle et signale `UNCERTAIN` si la borne haute dépasse le seuil. | effort élevé | risque moyen | gain 5
15. **Détection de rollback de l'horloge source** — a+b+d | Conserve le dernier horodatage par source, classe une horloge en recul comme `UNCERTAIN`. | effort moyen | risque moyen | gain 4
16. **Comparaison temporelle wrap-safe** — a+c | Soustraction modulo et règle de demi-période pour comparer des ticks 32/64 bits sans erreur au rollover. | effort moyen | risque faible | gain 4
17. **Ring SPSC C11 lock-free** — a+c+d | Ring producteur-consommateur unique à indices atomiques, refuse explicitement de publier quand il est plein. | effort élevé | risque élevé | gain 5
18. **Snapshot multi-lecteur par seqlock** — a+b+d | Plusieurs lecteurs prennent une photo cohérente d'un POD pendant qu'un écrivain le remplace, retries bornés. | effort élevé | risque moyen | gain 4
19. **Canaries et poisoning des tampons** — a+d+e | Redzones, canaries et remplissage `0xA5` dans les builds de diagnostic pour détecter corruption et lecture non initialisée sans ASan. | effort moyen | risque faible | gain 4
20. **Vues de mesure sans copie** — a+b+d | Vues empruntées porteuses d'un jeton de génération, évitant une copie par capteur tout en détectant les usages après invalidation. | effort moyen | risque moyen | gain 4
21. **Selftest sans état sur vecteurs de référence** ⭐ — a+d+e | Selftest sans état rejouant des vecteurs de référence versionnés (conversions, arithmétique, transitions) et retournant un bitset d'échec. | effort moyen | risque faible | gain 4
22. **Matrice de portabilité 32/64 bits et endianness** — a+c+d | Compile et exécute les mêmes vecteurs sous GCC et Clang en 32/64 bits et big/little endian via QEMU, layouts vérifiés au bit. | effort élevé | risque moyen | gain 5
23. **Profil C11 freestanding** — a+c+e | Compile le core freestanding sans libc ni horloge système, primitives mémoire/temps fournies par l'hôte. | effort élevé | risque moyen | gain 5
24. **Sanitizers complémentaires à ASan** ⭐ — a+d+e | Complète le fuzzing ASan par UBSan, MSan et TSan (débordements, lectures non initialisées, courses atomiques). | effort moyen | risque moyen | gain 5
25. **Injection de fautes déterministe aux frontières du core** — a+d | Injecte débordements, troncatures, retards et snapshots fissurés via des crochets de test pour vérifier que chaque frontière échoue sans écrire de mesure invalide. | effort moyen | risque faible | gain 5

## Lot B — Bus, drivers, simulation, scénarios

1. **Bus I2C multiplexé simulé** — a | Le publisher virtuel gère plusieurs capteurs sur un même bus I2C simulé avec adresses et collisions. | effort moyen | complexité du modèle | gain 4
2. **Modèle de latence de bus injectable** ⭐ — a+b | Chaque transaction du simulateur subit un délai configurable (NACK, clock stretching), rendant les mesures de latence réalistes sans matériel. | effort faible | latence simulée ≠ réelle | gain 4
3. **Timeline de scénario multi-événements** ⭐ — b+d | Un scénario JSON enchaîne plusieurs événements (panne, dérive, saturation, récupération) sur une timeline, plutôt qu'un seul motif. | effort moyen | scénario illisible si long | gain 4
4. **Capteurs corrélés simulés** — a | Le générateur produit des capteurs physiquement liés (température ↔ résistance) pour tester la plausibilité croisée. | effort moyen | corrélation artificielle | gain 4
5. **Glitches I2C injectables** — a+d | Le simulateur injecte NACK, bus bloqué et trames corrompues de façon reproductible par graine. | effort moyen | modèle simpliste | gain 4
6. **Modèle de bruit par capteur** — a | Bruit gaussien, quantification et hystérésis propres à chaque type, au lieu d'un bruit uniforme. | effort moyen | paramètres arbitraires | gain 3
7. **Simulateur de batterie réaliste** — b | Courbe de décharge Li-ion avec plateau et chute finale pour le publisher batterie. | effort faible | modèle simplifié | gain 3
8. **Simulateur IMU à trajectoire** — b+d | IMU 6 axes dont les valeurs dérivent d'une trajectoire scriptable (rotation, translation). | effort moyen | trajectoire non physique | gain 4
9. **Encodeur GPIO à fréquence variable** — a | Compteur d'impulsions sur GPIO simulé avec fréquence et sens variables. | effort faible | modèle idéal | gain 3
10. **Fichier thermique sysfs-like** — b | Le bus `file` lit un pseudo-fichier thermique simulé, reproduisant le sysfs Linux. | effort faible | spécifique Linux | gain 3
11. **Découverte par sondage actif** — c | Le publisher scanne le bus, identifie les capteurs et propose une description JSON prête à valider. | effort moyen | sondage perturbateur | gain 4
12. **Priorisation QoS sous saturation** — a | Les capteurs critiques sont publiés en premier quand le bus sature, les autres dégradés en `UNCERTAIN`. | effort moyen | politique contestable | gain 4
13. **Ordonnanceur de bus** — a | Planifie les lectures pour minimiser la latence globale et éviter les collisions d'un même bus. | effort élevé | optimal NP-difficile | gain 4
14. **Objet composite multi-capteurs** — b | Plusieurs capteurs publiés sous un objet parent commun (module thermique à 4 sondes). | effort moyen | arborescence complexe | gain 3
15. **Rejeu à vitesse variable** — b+d | Le rejeu d'un journal peut être accéléré, ralenti ou pas-à-pas, pour la démo et le debug. | effort faible | horodatage non respecté | gain 3
16. **Rejeu en boucle fermée** — a | Les pannes injectées dépendent des mesures précédentes, pour tester la récupération adaptative. | effort élevé | déterminisme difficile | gain 4
17. **Pannes aléatoires reproductibles** ⭐ — a+d | Générateur de pannes aléatoires mais reproductibles par graine, pour le fuzz de scénarios. | effort faible | couverture incomplète | gain 4
18. **Bus virtuel partagé multi-publishers** — a | Plusieurs publishers partagent un bus virtuel, pour tester concurrence et arbitrage. | effort élevé | concurrence simplifiée | gain 4
19. **Émulation depuis traces réelles** — b+e | Rejoue une capture réelle de capteur (CSV) comme source du publisher virtuel. | effort faible | traces non disponibles | gain 4
20. **Dérive thermique simulée** — a | La valeur dérive avec une température ambiante simulée, pour tester la calibration en service. | effort moyen | dérive linéaire | gain 3
21. **Saturation et clipping physiques** — a | Le capteur sature à ses bornes physiques et publie `SATURATED` au lieu de valeurs aberrantes. | effort faible | bornes mal déclarées | gain 3
22. **Détection de bus occupé** — a | Le publisher détecte une contention de bus et applique un repli, avec événement publié. | effort moyen | faux positifs | gain 3
23. **Multi-cadence sur bus unique** — a | Des capteurs à périodes différentes (1 Hz, 100 Hz) partagent un bus sans interférence. | effort moyen | gigue d'ordonnancement | gain 4
24. **Démarrage à froid marqué FIRST** — b | La première mesure après attachement porte le drapeau `FIRST`, visible dans le journal. | effort faible | déjà partiellement prévu | gain 2
25. **Calibration à point de référence injecté** — a | Le scénario injecte un point de référence connu pour recalculer les coefficients en direct. | effort moyen | référence absente sur cible | gain 4

## Lot C — Outillage hôte, observabilité, validation

1. **Mutation testing du core C11** ⭐ — a+d | `glane-mutate` injecte des mutants (bornes, énumérations, arithmétique) dans `core/`, reconstruit, rejoue les tests et bloque la CI sous un seuil de mutants tués. | effort moyen | mutants non équivalents | gain 5
2. **Porte de couverture de branches** ⭐ — a+d | `glane-cov` agrège les données `gcov` du core et des tests Python en Cobertura, puis exige un seuil sur les lignes modifiées. | effort moyen | seuil irréaliste | gain 4
3. **Analyse statique SARIF** — a+c | `glane-lint` combine `cppcheck`, `clang-tidy` et des règles de conversion entière sur `core/`, publie les écarts en SARIF. | effort moyen | faux positifs | gain 4
4. **Matrice de sanitizers du publisher natif** — a+d | `glane-san` rejoue le corpus hors fuzz du publisher sous ASan, UBSan et TSan sur un build hôte. | effort moyen | support incomplet | gain 4
5. **Réducteur de crashes de fuzz** — a+d | `glane-shrink` réduit par delta debugging une graine de crash jusqu'au plus petit JSON reproduisant la faute. | effort moyen | minimisation coûteuse | gain 4
6. **Fuzz continu OSS-Fuzz** — c+e | Variante C11 sans dépendance QNX adaptée à OSS-Fuzz sur Linux avec dictionnaire et corpus minimal. | effort élevé | infrastructure externe | gain 5
7. **Banc de saturation disque** — a+d | `glane-fill` remplit le volume de l'export circulaire pour déclencher ENOSPC et vérifier la récupération sans fichier tronqué. | effort moyen | ENOSPC différent sur QNX | gain 5
8. **Matrice de traçabilité GLA** ⭐ — b+d | `glane-trace` associe GLA-01..24 aux tests et aux étapes du codelab pour générer une matrice Markdown/HTML de preuves. | effort moyen | identifiants instables | gain 5
9. **Canonicalisation des artefacts JSON** ⭐ — a+e | `glane-canon` produit un JSON canonique (clés triées, UTF-8, entiers normalisés) pour descriptions et rapports, avec diff sémantique et empreinte. | effort faible | format ambigu | gain 3
10. **Linter de sortie PPS** — a+c | `glane-ppscheck` compare chaque enregistrement PPS au descripteur et signale champs absents et conversions silencieuses. | effort moyen | profil PPS mal documenté | gain 4
11. **Inspecteur de bundle QNX** — c+d | `glane-inspect` valide une archive de déploiement QNX (ELF, dépendances, scripts, modes, manifeste). | effort élevé | variantes du SDP | gain 5
12. **Audit du tree publié** — b+c | `glane-audit` contrôle sur la cible propriétaires, modes et liens de `/dev/sensors` face au manifeste. | effort moyen | permissions QNX | gain 4
13. **Vérification de build reproductible** ⭐ — a+e | `glane-repro` réalise deux builds propres avec `SOURCE_DATE_EPOCH` fixé et compare les empreintes de `core.a` et des outils. | effort moyen | build non hermétique | gain 5
14. **SBOM et provenance** — c+e | `glane-sbom` génère un SBOM SPDX JSON recensant core C11, outils Python, compilateur et SDP, avec licences et empreintes. | effort moyen | métadonnées QNX | gain 4
15. **Sélection de tests par impact** — d | `glane-impact` utilise un manifeste de dépendances pour sélectionner les tests affectés par chaque modification. | effort élevé | graphe incomplet | gain 4
16. **Profil critique des jobs CI** — d | `glane-ciprof` analyse les traces des jobs, produit une vue du chemin critique et signale toute étape > 60 % du budget. | effort moyen | traces variables | gain 4
17. **Détection et quarantaine des tests flaky** — d | `glane-flaky` rejoue les tests instables avec leurs graines et isole ceux qui échouent après trois tentatives. | effort moyen | faux positifs | gain 4
18. **Intégrité du cache CI** — d | `glane-cachecheck` lie chaque clé de cache aux empreintes des sources et vérifie les binaires réutilisés. | effort moyen | cache contaminé | gain 3
19. **Banc de disponibilité au boot** — b+d | `glane-boot` lance trente QEMU propres, mesure le délai jusqu'au premier `value` valide et publie p50/p95/échecs. | effort moyen | variabilité du boot | gain 5
20. **Banc thermique et énergétique Pi5** — b+d | `glane-thermal` pilote deux Pi5 à plusieurs cadences, corrèle puissance/throttling au p99 de publication. | effort élevé | conditions non comparables | gain 4
21. **Banc de soak 72 heures** — a+d | `glane-soak` exécute un banc nocturne alternant silence/STALE/SATURATED, mesure pente RSS et fuites. | effort élevé | interruption du banc | gain 5
22. **Garde de régression de performance** — d | `glane-perfgate` compare p99/CPU/RSS à une référence versionnée et bloque une régression > 10 %. | effort moyen | bruit des mesures | gain 5
23. **Rapport HTML hors ligne de campagne** ⭐ — b+e | `glane-report` transforme une capture d'exécution en HTML autonome (matrice qualité, histogrammes de latence, couverture). | effort moyen | captures volumineuses | gain 4
24. **Vérification exécutable de la documentation** — c+d | `glane-doccheck` exécute les commandes du README et du codelab dans une QEMU jetable et compare les sorties. | effort moyen | exemples non idempotents | gain 5
25. **Paquet Python `glane-tools`** — c+e | Les outils sont publiés en wheel/zipapp avec entrées `glane-*`, schémas versionnés et complétions shell, 100 % stdlib. | effort moyen | packaging divergent | gain 4

## Lot D — Écosystème QNX, interop, concours, marché

1. **Objet de commande audité** — c | Les commandes (`inject`, `calibrate`, `selftest`) sont protégées ET journalisées dans l'audit immuable. | effort moyen | surcoût d'écriture | gain 4
2. **Politique de sécurité à trois rôles** — c | Séparation publisher/admin/abonné dans une politique secpol type, au-delà du seul objet `command`. | effort moyen | politique spécifique | gain 4
3. **HAM avec backoff et escalade** — c | Un publisher en échec répétitif est relancé avec backoff puis signalé à un superviseur. | effort moyen | escalade non standard | gain 4
4. **Permissions par objet publié** — c | Chaque objet de l'arborescence porte ses propres permissions de lecture/écriture. | effort moyen | modèle QNX limité | gain 4
5. **Resource manager /dev/sensors** — a+c | Le publisher est un vrai `resmgr` avec entrées `/dev/sensors`, compatible avec les outils QNX standards. | effort élevé | complexité resmgr | gain 5
6. **Objets PPS natifs** — c | Les capteurs sont exposés comme objets PPS pour les abonnés QNX existants. | effort moyen | surcharge PPS | gain 4
7. **ionotify sur changement** — c | Les abonnés sont notifiés par `ionotify` uniquement quand la valeur change significativement. | effort moyen | seuil arbitraire | gain 3
8. **Pont MQTT Sparkplug B** — e | Les flux sont republiés en MQTT Sparkplug B pour l'Industrie 4.0. | effort élevé | hors périmètre QNX | gain 4
9. **Pont OPC UA** — e | Exposition des capteurs en OPC UA pour l'interopérabilité industrielle. | effort élevé | stack lourde | gain 4
10. **Export Prometheus/InfluxDB** — b+e | Les mesures sont poussées vers une base de télémétrie pour la supervision long terme. | effort moyen | dépendance externe | gain 3
11. **Support CAN bus** — a+e | Ajout du bus CAN (automobile) comme source de capteurs. | effort élevé | CAN hors périmètre vérifié | gain 4
12. **Dossier IEC 62304** — e | Traçabilité des exigences logicielles médicales mappée sur les tests Glane. | effort élevé | certification non auditée | gain 4
13. **Analyse ISO 26262** — e | Analyse de sécurité fonctionnelle automobile (ASIL) du pipeline capteurs. | effort élevé | claims non certifiées | gain 4
14. **Pitch équipementier auto** — d | Argumentaire ciblé zonale/ECU avec les chiffres de latence et de déterminisme. | effort faible | hors technique | gain 3
15. **Vidéo concours chapitrée** — d | Vidéo 3 min avec chapitres, sous-titres et preuves à l'écran. | effort faible | montage chronophage | gain 4
16. **Site GitHub Pages du projet** ⭐ — d | Page publique avec démo live, chiffres et architecture, pour le vote Community Choice. | effort moyen | maintenance du site | gain 4
17. **Badges build + couverture** — b | README avec badges CI, couverture et taille, signal de qualité immédiat. | effort faible | services tiers | gain 2
18. **Licence open core + commerciale** — e | Double licence : socle Apache-2.0, publishers certifiables sous licence commerciale. | effort moyen | modèle incertain | gain 3
19. **Exemple capteur amont gitlab QNX** — c | Proposition d'un exemple de capteur dans `gitlab.com/qnx/projects`. | effort moyen | acceptation incertaine | gain 4
20. **Documentation anglaise complète** ⭐ — d | Tout le README et les docs en anglais (règlement du concours). | effort faible | traduction fidèle | gain 4
21. **Tutoriel « porter un capteur en 10 min »** — b+d | Guide pas-à-pas pour ajouter un driver de capteur minimal. | effort moyen | tutoriel vite obsolète | gain 4
22. **Comparaison frameworks capteurs** — e | Tableau comparatif Glane vs ROS 2 vs Zephyr sensors, positionnement honnête. | effort faible | comparaison biaisée | gain 3
23. **Intégration Yocto/meta-qnx** — e | Recette Yocto pour embarquer Glane dans une image QNX construite. | effort élevé | meta-qnx peu documenté | gain 3
24. **Support multi-cibles** — c | Pi5, QEMU x86 et SabreLite avec la même base de code. | effort moyen | cibles non disponibles | gain 3
25. **Feuille de route produit post-concours** — e | Plan 3 mois → 1 an : libglane publique, certification, support. | effort faible | roadmap non tenue | gain 3

---

## 5. Les 10 meilleures — sélectionnées et implémentées

Critères : **gratuit** (aucun SDP, matériel, ni API payante), **réalisable sur Linux
maintenant**, **forte valeur** pour le concours et le produit, **testable**.

| # | Idée | Lot | Pourquoi maintenant |
|---|---|---|---|
| 1 | **Jumeau Python de calibration + test différentiel C↔Python** | A12 | Prouve le rejeu bit à bit (GLA-05), l'argument central « mesurer plutôt qu'affirmer » |
| 2 | **Bus virtuel + scénarios + rejeu déterministe** | B2/B3 | Le simulateur est à la fois démo, harnais CI et preuve avant d'avoir une carte (GLA-04) |
| 3 | **Arithmétique saturante contractuelle** | A1 | Transforme un débordement en `SATURATED` traçable, jamais de valeur fausse silencieuse |
| 4 | **Selftest sans état sur vecteurs de référence** | A21 | Base de la commande `selftest` publiée et du diagnostic terrain |
| 5 | **`glane-mutate` — mutation testing** | C1 | ≥ 8 mutants tués par les tests (plan de vérification §7.5) |
| 6 | **`glane-trace` — matrice de traçabilité GLA** | C8 | Relie chaque exigence aux tests et preuves : dossier de soumission |
| 7 | **`gschema` — validateur d'arborescence hôte** | C | Comble le trou PUB-CONF (GLA-08), rejouable par un juge |
| 8 | **`glat` — banc de latence** | C | Produit les p50/p99/p99.9 réels (GLA-09), aucun chiffre annoncé sans mesure |
| 9 | **`glane-report` — rapport HTML hors ligne** | C23 | Visualisation autonome d'une campagne : qualité, latence, pertes |
| 10 | **`glane-canon` — canonicalisation JSON + empreinte** | C9 | Empreinte stable des descriptions pour le rejeu déterministe et le diff sémantique |

Les 10 sont implémentées, testées et poussées sur le dépôt — voir `README.md`
et les commits correspondants.