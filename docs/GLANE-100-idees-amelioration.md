# Glane — 100 idées pour améliorer le projet (état réel, 12/11/2026)

État au moment de l'écriture : socle `core/` C11 pur (973 checks), outils hôte
Python (gschema/glat/greport/gcanon/glane_twin/gjournal), `glaned` **compilé et
exécuté pour de vrai sur QNX 8.0** (émulateur `qemu -kernel`, IFS minimale,
publication d'objets dans `/dev/shmem`), site GitHub Pages. Ce qui n'est **pas**
encore fait : vrai PPS/resmgr, HAM, secpol, capteurs réels multiples, latence
mesurée sur cible, fuzz, vidéo, codelab.

Groupées en 10 thèmes × 10. Format : `N. idée — impact (effort)`.

## 1. Mesure & calibration (core)
1. Unités étendues (m/s², rad/s, lux, hPa, mbar, Vrms…) — couverture marché (faible)
2. Conversion d'unités entière (C↔K, V↔mV) sans flottant — zéro flottant garanti (faible)
3. Calibration par morceaux (table + interpolation entière) — capteurs non linéaires (moyen)
4. Hystérésis de calibration — anti-rebond de switching (faible)
5. Domaine de validité de calibration — refuse l'extrapolation hors plage (faible)
6. Incertitude propagée par mesure → `UNCERTAIN` — crédibilité métrologique (moyen)
7. Seuil de bruit/SNR par capteur dans la description — détection de dérive (faible)
8. Gigue de période mesurée (réel vs déclaré) — honnêteté de cadence (moyen)
9. Époque/transaction de séquence (distinguer reprise vs perte) — rejeu robuste (moyen)
10. Signature canonique de description en C (exposer gcanon dans core) — détection de changement (faible)

## 2. Bus & drivers
11. Bus I2C simulé multi-périphériques (adresses, collisions) — réalisme (moyen)
12. Latence de bus injectable par transaction — mesures de latence crédibles (faible)
13. Glitches I2C (NACK, clock stretching, bus stuck) reproductibles — fault injection (moyen)
14. Émulation SPI (MOSI/MISO, chip select) — couverture bus (moyen)
15. GPIO compteur d'impulsions (encodeur) à fréquence variable — publisher encodeur (faible)
16. Fichier thermique sysfs-like (bus `file`) — portage Linux réaliste (faible)
17. Découverte par balayage (WHO_AM_I) + proposition de description — auto-config (moyen)
18. Récupération de bus : backoff, blacklist, réanalyse — fiabilité (moyen)
19. Arbitrage des lectures sur bus partagé — éviter les collisions (élevé)
20. Multi-cadence hétérogène (1 Hz + 100 Hz) sur un bus — réalisme (moyen)

## 3. Publication QNX (le cœur du concours)
21. **PPS réel** : objets `pps`/io-pkt au lieu de fichiers — techno QNX native (moyen)
22. **resmgr `/dev/sensors`** : vrai gestionnaire de ressources — la voie Low-level (élevé)
23. Notification `ionotify` aux abonnés sur changement — réactivité (moyen)
24. Objet `command` protégé (inject/calibrate/selftest) + audit — sûreté (moyen)
25. **Supervision HAM** : glaned tué → relancé, reprise mesurée — techno QNX (moyen)
26. Politique `secpol` type (publisher/abonné/admin) — sécurité (moyen)
27. Permissions par objet publié — granularité (moyen)
28. Abonné multiple + compteur de pertes (GLA-16) — preuve sans perte (moyen)
29. File/ring bornée avant notification — contre-pression (moyen)
30. Publication par lot (plusieurs mesures/notification) — 1 kHz soutenable (élevé)

## 4. Démo émulateur & scénarios
31. **Multi-capteurs réels simultanés** (temp + IMU + courant) — prouve le framework (moyen)
32. **Scénario chaos en boucle fermée** (panne → récupération visible) — actif démo (moyen)
33. Player de scénarios JSON multi-événements (timeline) — reproductibilité (moyen)
34. Rejeu accéléré/ralenti/pas-à-pas — debug + démo (faible)
35. Capture asciinema de la session QNX embarquée au site — preuve vivante (faible)
36. Screenshot console → PNG dans la section émulateur — visible (faible)
37. Oscilloscope web (WebSocket) — vote Community Choice (élevé)
38. Démo « un capteur réel » sur Pi5 — si matériel dispo (moyen)
39. Démo « je tue glaned → il renaît » (HAM à l'écran) — persuasif (faible)
40. Journal QNX complet téléchargeable (pas seulement l'extrait) — reproductibilité (faible)

## 5. Performance & QoS
41. **Latence mesurée sur la cible réelle** (pas simulée) — cœur de la promesse (moyen)
42. Banc de charge 1 kHz cumulé sans perte (GLA-16 mesuré) — chiffre (moyen)
43. Priorités/partitions de threads (SCHED_FIFO) pour glaned — déterminisme (moyen)
44. Budget de publication par capteur + dégradation déclarée — QoS (moyen)
45. Chiffre TCG vs KVM pour la démo — honnêteté (faible)
46. Profil mémoire/RSS de glaned (pas de fuite) — qualité (faible)
47. Latence de boot (QEMU → premier objet valide) — chiffre (moyen)
48. Courbe p99 vs nombre de capteurs — scalabilité (moyen)
49. Ring SPSC lock-free — perf (élevé)
50. Gigue de publication mesurée — précision (moyen)

## 6. Outils hôte & observabilité
51. gplot : courbes SVG/PNG depuis le journal — visualisation (moyen)
52. glat : latence dans le temps (pas que percentiles) — diagnostique (faible)
53. greport : diff de deux campagnes (avant/après) — comparaison (moyen)
54. gschema : valider aussi le contenu (unités, plages) — rigueur (moyen)
55. gcanon : diff sémantique de descriptions — revue (faible)
56. glane-trace : lien GLA↔test↔commit — traçabilité git (moyen)
57. gjournal : requête/filtre d'un journal (qualité, plage) — exploration (moyen)
58. Export CSV/Prometheus — supervision (faible)
59. greport en mode watch (rafraîchi en direct) — dev UX (moyen)
60. clang-format + lint C en CI — cohérence (faible)

## 7. Validation, fuzz, qualité
61. **Fuzz 100 000 entrées** (grammaire + mutation) du parseur JSON — robustesse (moyen)
62. TSan en plus d'ASan/UBSan — courses de threads (moyen)
63. Couverture (gcov) en badge — signal qualité (faible)
64. Mutation testing sur la couche publication (pas que core) — rigueur (moyen)
65. Build reproductible (hash stable de l'IFS) — vérifiability (moyen)
66. SBOM / SPDX du dépôt — conformité (faible)
67. Test du boot QNX en CI (runner self-hosted + SDP) — automatisé (élevé)
68. Chaos : tuer glaned, corrompre desc, remplir disque — résilience (moyen)
69. Vérification d'absence de fuite de fds/objets — qualité (moyen)
70. Golden files (journaux figés comparés) — régression déterminisme (faible)

## 8. Docs, codelab, soumission, site
71. **README anglais complet** — règlement (faible)
72. **Vidéo 3 min scriptée** — livrable obligatoire (moyen)
73. Codelab 4 chapitres auto-corrigé — bonus 3 500 $ (élevé)
74. Page Devpost prête (description, build/run, evidence) — soumission (faible)
75. Site : section « reproduire en 1 commande » — adoption (faible)
76. Site : badges CI/tests/couverture — crédibilité (faible)
77. Site : traduction EN — audience (moyen)
78. Diagramme d'architecture illustré — clarté (faible)
79. FAQ concours (pourquoi latence bornée, pourquoi QNX) — argumentaire (faible)
80. Cas d'usage industriel concret (argument impact) — marché (faible)

## 9. Interop & standards
81. Pont PPS → abonné existant (montrer l'interop QNX native) — démo (moyen)
82. Export JSON → MQTT (Industrie 4.0) — interop (moyen)
83. Nomenclature médicale IEEE 11073 (codes MDC) — marché médical (moyen)
84. Pont ROS 2 (ros2 porté sur QNX) — robotique (élevé)
85. CAN bus (automobile) — source capteur (élevé)
86. Export OPC UA — industriel (élevé)
87. Flux invité Linux (hyperviseur, mémoire partagée) — mixed-criticality (élevé)
88. Conteneur OCI QNX — déploiement (moyen)
89. Bridge InfluxDB/Prometheus — supervision (moyen)
90. Streaming SSE pour un dashboard hôte — dev UX (moyen)

## 10. Écosystème, amont, produit
91. **libglane** : API publique documentée pour écrire un publisher — le trou « aucune API » (élevé)
92. Exemple amont QNX (gitlab.com/qnx) — contribution (moyen)
93. Feuille de route post-concours (3 mois → 1 an) — continuité (faible)
94. Double licence open core/commerciale — business (moyen)
95. Benchmark vs Zephyr sensors / Linux IIO — positionnement (moyen)
96. Intégration Yocto/meta-qnx — distribution (élevé)
97. Support multi-cibles (Pi5, QEMU x86, SabreLite) — portabilité (moyen)
98. **CI build QNX sur runner self-hosted** — preuve automatisée (élevé)
99. `glane doctor` (diagnostic en 1 commande) — produit ≠ bibliothèque (moyen)
100. Packaging wheel/zipapp des outils hôte (`glane-tools`) — distribution (moyen)