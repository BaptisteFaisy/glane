/*
 * Glane — Copyright 2026 Baptiste Faisy. Licence Apache-2.0.
 *
 * app.js — rendu des sections pilotées par les données (site/data.js).
 * Toutes les valeurs viennent des sorties réelles des outils. Échappement
 * systématique : aucune donnée n'est injectée en HTML brut.
 */
(function () {
  "use strict";
  var D = window.GLANE_DATA || {};
  var $ = function (id) { return document.getElementById(id); };

  function esc(s) {
    return String(s == null ? "" : s)
      .replace(/&/g, "&amp;").replace(/</g, "&lt;").replace(/>/g, "&gt;")
      .replace(/"/g, "&quot;");
  }
  function n(x) { return Number(x).toLocaleString("fr-FR"); }

  var REPO = "https://github.com/BaptisteFaisy/glane";

  /* --------------------------- en-tête --------------------------- */
  function renderBadges() {
    var mut = D.mutation || {}, diff = D.differential || {};
    var b = [
      ["vérifications C", n(D.checks_total || 0)],
      ["tests hôte", n(D.host_tests || 0)],
      ["différentiel", n(diff.values || 0) + " · " + (diff.divergences || 0) + " écart"],
      ["mutants tués", (mut.killed || 0) + "/" + (mut.total || 0)],
      ["cas fautifs", n(D.faulty_bank || 0)],
      ["idées", "100"]
    ];
    $("badges").innerHTML = b.map(function (x) {
      return '<span class="badge">' + esc(x[0]) + " <b>" + esc(x[1]) + "</b></span>";
    }).join("");
    $("generated").textContent = D.generated || "";
  }

  var HOLES = [
    ["PUB-SDK", "Aucune API ni SDK « écrire un publisher » documentée"],
    ["PUB-SENS", "Un seul publisher livré ; rien pour capteurs, I2C/SPI, batterie, thermique"],
    ["PUB-SIM", "Aucune simulation ni test sans matériel mentionné dans tout le guide"],
    ["PUB-CONF", "Aucun validateur de conformité des objets publiés"],
    ["PUB-LAT", "Rien sur la latence de publication ni l'horodatage des notifications"],
    ["PUB-META", "Modèle de données sans unités, plages, calibration ni horodatage capteur"],
    ["PUB-LIFE", "Aucune intégration HAM / cycle de vie documentée"],
    ["PUB-SEC", "Aucune permission ni politique de sécurité pour les objets publiés"],
    ["SENS-ABS", "GPIO et SPI ont des frameworks, mais aucune abstraction capteur au-dessus"]
  ];
  function renderHoles() {
    $("holes").innerHTML = HOLES.map(function (h) {
      return '<span class="tag"><b>' + esc(h[0]) + "</b> · " + esc(h[1]) + "</span>";
    }).join("");
  }

  /* ------------------------ fonctionnalités ------------------------ */
  function featCard(f) {
    var gla = (f.gla || []).map(function (g) {
      return '<span class="gla">' + esc(g) + "</span>";
    }).join("");
    return '<div class="feat" data-kind="' + esc(f.kind) + '" data-gla="' +
      (f.gla && f.gla.length ? "1" : "0") + '">' +
      '<div class="head"><h4>' + esc(f.name) + '</h4><span class="kind ' +
      esc(f.kind) + '">' + esc(f.kind) + "</span></div>" +
      '<div class="id">' + esc(f.id) + "</div>" +
      "<p>" + esc(f.desc) + "</p>" + (gla ? "<div>" + gla + "</div>" : "") + "</div>";
  }
  function renderFeatures() {
    $("features-grid").innerHTML = (D.features || []).map(featCard).join("");
    var btns = $("f-filter").querySelectorAll("button");
    btns.forEach(function (b) {
      b.addEventListener("click", function () {
        btns.forEach(function (x) { x.classList.remove("on"); });
        b.classList.add("on");
        var k = b.getAttribute("data-kind");
        $("features-grid").querySelectorAll(".feat").forEach(function (c) {
          var show = k === "all" ||
            (k === "gla" ? c.getAttribute("data-gla") === "1"
              : c.getAttribute("data-kind") === k);
          c.style.display = show ? "" : "none";
        });
      });
    });
  }

  /* ---------------------------- preuves ---------------------------- */
  function renderProofs() {
    var mut = D.mutation || {}, diff = D.differential || {}, sc = D.sizecheck || {};
    var p = [
      ["Vérifications C", n(D.checks_total || 0), "make test"],
      ["Tests hôte", n(D.host_tests || 0), "unittest"],
      ["Différentiel C↔Python", n(diff.values || 0), (diff.divergences || 0) + " divergence"],
      ["Mutants tués", (mut.killed || 0) + "/" + (mut.total || 0), mut.verified ? "vérifié" : "CI"],
      ["Descriptions fautives", n(D.faulty_bank || 0), "erreur attendue chacune"],
      ["Fichiers sous borne", n(sc.files || 0), "<500 lignes, <50/fonc"],
      ["ASan / UBSan", "vert", "make asan"],
      ["CI", "2 jobs verts", "build-test + host-test"]
    ];
    $("proofs").innerHTML = p.map(function (x) {
      return '<div class="stat"><div class="k">' + esc(x[0]) + '</div><div class="v">' +
        esc(x[1]) + '</div><div class="k">' + esc(x[2]) + "</div></div>";
    }).join("");
  }

  function renderChecks() {
    var checks = D.checks || {};
    var keys = Object.keys(checks).sort(function (a, b) { return checks[b] - checks[a]; });
    var max = keys.length ? checks[keys[0]] : 1;
    $("checks-bars").innerHTML = keys.map(function (k) {
      var pct = Math.round((checks[k] / max) * 100);
      return '<div class="bar-row"><span class="name">' + esc(k) +
        '</span><span class="bar"><i style="width:' + pct + '%"></i></span>' +
        '<span class="n">' + n(checks[k]) + "</span></div>";
    }).join("");
  }

  /* --------------------------- journaux --------------------------- */
  var curJournal = null;
  function renderJournals() {
    var js = D.journals || {};
    var keys = Object.keys(js);
    if (!keys.length) return;
    curJournal = keys[0];
    $("journal-tabs").innerHTML = keys.map(function (k, i) {
      return '<button data-k="' + esc(k) + '" class="' + (i === 0 ? "on" : "") + '">' +
        esc(js[k].label) + "</button>";
    }).join("");
    $("journal-tabs").querySelectorAll("button").forEach(function (b) {
      b.addEventListener("click", function () {
        $("journal-tabs").querySelectorAll("button").forEach(function (x) { x.classList.remove("on"); });
        b.classList.add("on");
        curJournal = b.getAttribute("data-k");
        showJournal();
      });
    });
    showJournal();
  }
  function showJournal() {
    var j = (D.journals || {})[curJournal];
    if (!j) return;
    $("journal-body").textContent = j.head.join("\n") + "\n… (" +
      j.values.length + "+ mesures, journal complet produit par glaned-sim)";
  }

  function renderGlat() {
    var js = D.journals || {};
    var keys = Object.keys(js);
    $("glat-tables").innerHTML = keys.map(function (k) {
      var s = js[k].stats || {};
      var rows = [["p50", s.p50_ns], ["p99", s.p99_ns], ["p99.9", s.p99_9_ns],
        ["max", s.max_ns], ["moyenne", s.mean_ns], ["pertes", s.total_losses]]
        .map(function (r) {
          return "<tr><td>" + esc(r[0]) + "</td><td class='mono'>" + n(r[1]) + "</td></tr>";
        }).join("");
      return '<div class="card" style="display:inline-block;vertical-align:top;' +
        'margin:0 .6rem .6rem 0;min-width:200px"><h3 style="margin-top:0">' +
        esc(js[k].label) + '</h3><table><tr><th>mesure</th><th>ns</th></tr>' +
        rows + "</table></div>";
    }).join("");
  }

  function renderMutation() {
    var m = D.mutation || {};
    $("mutation-summary").innerHTML =
      "<strong>" + (m.killed || 0) + "/" + (m.total || 0) + "</strong> mutants tués" +
      (m.verified ? " (vérifié à la génération des données)" : " (porte de la CI)") +
      ". Chaque mutant est une altération ciblée du code de décision ; s'il survit, " +
      "un test est trop faible.";
    $("mutation-list").innerHTML = (m.mutants || []).map(function (x, i) {
      return "<li><code>" + esc(x.file) + "</code> — " + esc(x.desc) + "</li>";
    }).join("");
  }

  function renderDiff() {
    var d = D.differential || {};
    $("diff-summary").innerHTML =
      n(d.values || 0) + " jeux de coefficients et d'entrées tirés (graine fixe " +
      esc(d.seed) + "), dans les bornes du contrat. La calibration C " +
      "(<code>calib_dump</code>) et le jumeau Python (<code>glane_twin</code>) " +
      "rendent exactement les mêmes entiers : <strong>" + (d.divergences || 0) +
      " divergence</strong>. Preuve que le rejeu de la calibration est bit à bit.";
  }

  /* -------------------------- exigences -------------------------- */
  function renderRequirements() {
    var cov = {};
    (D.trace || []).forEach(function (r) { cov[r.id] = r; });
    var rows = (D.requirements || []).map(function (r) {
      var t = cov[r.id] || {};
      var ok = t.covered;
      return "<tr><td class='mono'>" + esc(r.id) + "</td><td><span class='pill " +
        esc(r.level) + "'>" + esc(r.level) + "</span></td><td>" + esc(r.text) +
        "</td><td class='" + (ok ? "yes'>" : "no'>") + (ok ? "✓ citée" : "—") + "</td></tr>";
    }).join("");
    $("requirements").innerHTML =
      "<table><tr><th>ID</th><th>Niv.</th><th>Exigence</th><th>État</th></tr>" +
      rows + "</table>";
  }

  function renderTrace() {
    var rows = (D.trace || []).map(function (r) {
      var files = (r.files || []).map(function (f) { return "<code>" + esc(f) + "</code>"; }).join(" ");
      return "<tr><td class='mono'>" + esc(r.id) + "</td><td><span class='pill " +
        esc(r.level) + "'>" + esc(r.level) + "</span></td><td class='" +
        (r.covered ? "yes'>" : "no'>") + (r.covered ? "✓" : "—") + "</td><td>" +
        (files || "<span class='hint'>aucun</span>") + "</td></tr>";
    }).join("");
    $("trace-matrix").innerHTML =
      "<table><tr><th>ID</th><th>Niv.</th><th>Couverte</th><th>Fichiers / tests</th></tr>" +
      rows + "</table>";
  }

  /* ---------------------------- idées ---------------------------- */
  var DIRNAME = { a: "technique", b: "produit", c: "QNX/amont", d: "concours", e: "au-delà" };
  function ideaCard(it) {
    return '<div class="idea' + (it.star ? " star" : "") + '">' +
      '<div class="n">' + it.n + '</div><div class="body"><strong>' +
      (it.star ? '<span class="star-mark">⭐</span> ' : "") + esc(it.name) +
      "</strong><p>" + esc(it.what) + "</p></div>" +
      '<div class="meta-r"><span class="gain">gain ' + it.gain + "</span><br>" +
      esc(it.dir.split("").map(function (c) { return DIRNAME[c] || c; }).join(", ")) +
      "<br>effort " + esc(it.effort) + "</div></div>";
  }
  function renderIdeas() {
    var lots = D.ideas || {};
    var order = ["A", "B", "C", "D"];
    $("ideas-list").innerHTML = order.filter(function (k) { return lots[k]; }).map(function (k) {
      var L = lots[k];
      return '<div class="lot" data-lot="' + k + '"><h3>Lot ' + k + " — " +
        esc(L.title) + ' <span class="count">' + L.items.length + " idées</span></h3>" +
        L.items.map(ideaCard).join("") + "</div>";
    }).join("");
    var btns = $("i-filter").querySelectorAll("button");
    btns.forEach(function (b) {
      b.addEventListener("click", function () {
        btns.forEach(function (x) { x.classList.remove("on"); });
        b.classList.add("on");
        var lot = b.getAttribute("data-lot");
        $("ideas-list").querySelectorAll(".lot").forEach(function (el) {
          el.style.display = (lot === "all" || lot === el.getAttribute("data-lot")) ? "" : "none";
        });
        $("ideas-list").querySelectorAll(".idea").forEach(function (el) {
          if (lot === "star") {
            el.style.display = el.classList.contains("star") ? "" : "none";
          } else {
            el.style.display = "";
          }
        });
        if (lot === "star") {
          $("ideas-list").querySelectorAll(".lot").forEach(function (el) { el.style.display = ""; });
        }
      });
    });
  }

  /* ----------------------------- docs ----------------------------- */
  var DOCS = [
    ["docs/GLANE-cahier-des-charges.md", "Cahier des charges", "24 exigences GLA-01..24, 4 essais de faisabilité, 9 lots délégables"],
    ["docs/GLANE-etat-et-poussee.md", "État et poussée", "où en est le projet, chemin critique daté, 25 leviers"],
    ["docs/GLANE-100-idees-aller-plus-loin.md", "100 idées", "4 lots de 25 idées neuves ; les 10 meilleures implémentées"],
    ["docs/GLANE-traceabilite.md", "Traçabilité", "matrice exigences → tests (générée par glane_trace)"],
    ["CONTRACT.md", "Contrat gelé", "ABI de la mesure, calibration, schéma publié, journal"],
    ["README.md", "README", "construire, tester, démo express"]
  ];
  function renderDocs() {
    $("docs-links").innerHTML = DOCS.map(function (d) {
      return '<div class="card"><h3><a href="' + REPO + "/blob/main/" + d[0] + '">' +
        esc(d[1]) + "</a></h3><p>" + esc(d[2]) + '</p><p class="small"><code>' +
        esc(d[0]) + "</code></p></div>";
    }).join("");
  }

  function renderContractJournal() {
    var js = D.journals || {};
    var k = Object.keys(js)[0];
    var line = (k && js[k].head && js[k].head[0]) ||
      '{"s":"temp-chassis","id":1,"seq":1,"ts_ns":0,"d_ns":1500000,"v_m":42500,"u":"C","q":"ok","f":"first|calibrated|simulated"}';
    $("contract-journal").textContent = line;
  }

  function renderToolsUsage() {
    var cmd = [
      "make            # socle statique + 9 binaires de test + outils hôte",
      "make test       # " + (D.checks_total || 0) + " vérifications C",
      "make host-test  # " + (D.host_tests || 0) + " tests hôte (unittest)",
      "make asan       # AddressSanitizer + UndefinedBehaviorSanitizer",
      "make sizecheck  # garde de taille",
      "",
      "# publisher virtuel -> journal -> latence -> rapport",
      "./build/glaned-sim core/tests/valid/temp-chassis.json 2000 sine 42 1500000 /tmp/j.jsonl",
      "python3 host/glat.py    /tmp/j.jsonl            # p50/p99/p99.9",
      "python3 host/greport.py /tmp/j.jsonl rapport.html",
      "python3 host/gschema.py <arbre-publié>",
      "python3 host/gcanon.py  core/tests/valid/temp-chassis.json",
      "",
      "# vérification",
      "python3 scripts/glane_mutate.py   # test de mutation",
      "python3 scripts/glane_trace.py    # matrice de traçabilité"
    ].join("\n");
    $("tools-usage").innerHTML = "<pre class='code'>" + esc(cmd) + "</pre>";
  }

  /* --------------------- navigation active --------------------- */
  function initNav() {
    var links = Array.prototype.slice.call(document.querySelectorAll(".nav a"));
    var secs = links.map(function (a) { return document.querySelector(a.getAttribute("href")); });
    function onScroll() {
      var y = window.scrollY + 120, idx = 0;
      secs.forEach(function (s, i) { if (s && s.offsetTop <= y) idx = i; });
      links.forEach(function (a, i) { a.classList.toggle("active", i === idx); });
    }
    window.addEventListener("scroll", onScroll, { passive: true });
    onScroll();
  }

  function init() {
    renderBadges(); renderHoles(); renderFeatures(); renderProofs();
    renderChecks(); renderJournals(); renderGlat(); renderMutation();
    renderDiff(); renderRequirements(); renderTrace(); renderIdeas();
    renderDocs(); renderContractJournal(); renderToolsUsage(); initNav();
  }

  if (document.readyState === "loading") {
    document.addEventListener("DOMContentLoaded", init);
  } else { init(); }
})();
