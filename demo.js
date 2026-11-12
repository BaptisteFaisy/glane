/*
 * Glane — Copyright 2026 Baptiste Faisy. Licence Apache-2.0.
 *
 * demo.js — démo interactive du publisher virtuel.
 *
 * Portage fidèle de core/src/virtuel.c (formes d'onde, hachage de bruit,
 * saturation) et de core/src/calib.c (Horner + arrondi demi-à-l'infini, en
 * BigInt pour rester exact). Aucune dépendance externe. Ce que le visiteur
 * voit est ce que produit le C, aux paramètres près.
 */
(function () {
  "use strict";

  /* Table sinusoïdale Q15 — identique à virtuel.c. */
  var SINE = [
    0, 3212, 6393, 9512, 12539, 15446, 18204, 20787,
    23170, 25329, 27245, 28898, 30273, 31356, 32137, 32609,
    32767, 32609, 32137, 31356, 30273, 28898, 27245, 25329,
    23170, 20787, 18204, 15446, 12539, 9512, 6393, 3212,
    0, -3212, -6393, -9512, -12539, -15446, -18204, -20787,
    -23170, -25329, -27245, -28898, -30273, -31356, -32137, -32609,
    -32767, -32609, -32137, -31356, -30273, -28898, -27245, -25329,
    -23170, -20787, -18204, -15446, -12539, -9512, -6393, -3212
  ];
  var INT32_MAX = 2147483647, INT32_MIN = -2147483648;
  var SCALE = 1000000n;
  var QNAME = ["ok", "stale", "saturated", "fault", "uncertain"];

  /* Hachage de bruit — identique à hash_noise() de virtuel.c (uint32). */
  function hashNoise(seed, step) {
    var h = (seed ^ Math.imul(step, 2654435761)) >>> 0;
    h = (h ^ (h >>> 16)) >>> 0;
    h = (Math.imul(h, 0x7feb352d)) >>> 0;
    h = (h ^ (h >>> 15)) >>> 0;
    h = (Math.imul(h, 0x846ca68b)) >>> 0;
    h = (h ^ (h >>> 16)) >>> 0;
    return h & 0x7fff;
  }

  /* Forme d'onde — identique à glane_virtuel_wave (divisions entières exactes
   * via BigInt pour coller à la troncature C). */
  function wave(cfg, step) {
    var amp = BigInt(cfg.amp), per = BigInt(cfg.period > 0 ? cfg.period : 1);
    var phase = per > 0n ? BigInt(step) % per : 0n;
    if (cfg.wf === 0) return 0;
    if (cfg.wf === 1) {
      var idx = Number(((BigInt(step) * 64n) / per) % 64n);
      return Number((amp * BigInt(SINE[idx])) / 32767n);
    }
    if (cfg.wf === 2) {
      return Number((amp * (2n * phase - per)) / per);
    }
    if (cfg.wf === 3) {
      var half = per / 2n;
      if (half <= 0n) return 0;
      if (phase < half) return Number(-amp + (2n * amp * phase) / half);
      var rest = per - half;
      return Number(amp - (2n * amp * (phase - half)) / (rest > 0n ? rest : 1n));
    }
    if (cfg.wf === 4) {
      var r = BigInt(hashNoise(cfg.seed, step));
      return Number((r * amp) / 16384n) - cfg.amp;
    }
    return 0;
  }

  /* Calibration — identique à glane_calib_apply (Horner + demi à l'infini). */
  function calibApply(coef, deg, x) {
    if (deg < 0) deg = 0;
    if (deg > 3) deg = 3;
    var acc = coef[deg], xb = BigInt(x), i;
    for (i = deg - 1; i >= 0; i--) acc = acc * xb + coef[i];
    var half = SCALE / 2n;
    var num = acc >= 0n ? acc + half : acc - half;
    var q = num / SCALE;            /* BigInt tronque vers zéro, comme C */
    if (q > BigInt(INT32_MAX)) return INT32_MAX;
    if (q < BigInt(INT32_MIN)) return INT32_MIN;
    return Number(q);
  }

  /* Percentile par rang le plus proche — identique à glat.py. */
  function percentile(sorted, p) {
    var n = sorted.length;
    if (!n) return 0;
    var rank = Math.ceil((p / 100) * n);
    if (rank < 1) rank = 1;
    if (rank > n) rank = n;
    return sorted[rank - 1];
  }

  /* FNV-1a 32 bits — empreinte du journal pour la démo de déterminisme. */
  function fnv1a(str) {
    var h = 0x811c9dc5, i;
    for (i = 0; i < str.length; i++) {
      h ^= str.charCodeAt(i);
      h = Math.imul(h, 0x01000193) >>> 0;
    }
    return ("0000000" + h.toString(16)).slice(-8);
  }

  /* Simule `steps` mesures. Retourne {samples, journal, fp, ages}. */
  function simulate(cfg) {
    var samples = [], lines = [], ages = [], step, raw, q, val, s;
    var staleNs = BigInt(cfg.staleMs) * 1000000n;
    var calibOn = cfg.calibKind === "poly";
    for (step = 0; step < cfg.steps; step++) {
      raw = cfg.base + wave(cfg, step) + cfg.drift * step;
      q = 0; /* OK */
      if (cfg.satMin < cfg.satMax) {
        if (raw < cfg.satMin) { raw = cfg.satMin; q = 2; }
        else if (raw > cfg.satMax) { raw = cfg.satMax; q = 2; }
      }
      if (raw > INT32_MAX) { raw = INT32_MAX; q = 2; }
      else if (raw < INT32_MIN) { raw = INT32_MIN; q = 2; }
      val = calibOn ? calibApply(cfg.coef, cfg.deg, raw) : raw;
      var ts = BigInt(step) * 1000000n;            /* 1 ms par pas */
      var jit = cfg.jitter > 0 ? (hashNoise(cfg.seed + 1, step) % (cfg.jitter + 1)) : 0;
      var age = BigInt(cfg.latency + jit);
      var dn = ts + age;
      if (q === 0 && age > staleNs) q = 1;          /* STALE */
      var parts = [];
      if (step === 0) parts.push("first");
      if (calibOn) parts.push("calibrated");
      parts.push("simulated");
      var flags = parts.join("|");
      s = { seq: step + 1, ts: ts, dn: dn, v: val, q: q, f: flags };
      samples.push(s);
      ages.push(Number(age));
      lines.push('{"s":"' + (cfg.name || "demo") + '","id":1,"seq":' + s.seq +
        ',"ts_ns":' + ts + ',"d_ns":' + dn + ',"v_m":' + val + ',"u":"C","q":"' +
        QNAME[q] + '","f":"' + flags + '"}');
    }
    var journal = lines.join("\n");
    return { samples: samples, journal: journal, fp: fnv1a(journal), ages: ages };
  }

  /* ----------------------------- rendu ----------------------------- */
  var $ = function (id) { return document.getElementById(id); };

  function renderChart(samples, cfg) {
    var W = 720, H = 240, pad = 6;
    var vals = samples.map(function (s) { return s.v; });
    var lo = Math.min.apply(null, vals.concat([cfg.satMin]));
    var hi = Math.max.apply(null, vals.concat([cfg.satMax]));
    if (hi === lo) { hi = lo + 1; }
    var span = hi - lo;
    function X(i) { return pad + (i / Math.max(1, samples.length - 1)) * (W - 2 * pad); }
    function Y(v) { return H - pad - ((v - lo) / span) * (H - 2 * pad); }
    var pts = samples.map(function (s, i) {
      return X(i).toFixed(1) + "," + Y(s.v).toFixed(1);
    }).join(" ");
    var satLo = Y(cfg.satMin).toFixed(1), satHi = Y(cfg.satMax).toFixed(1);
    var svg = '<line x1="0" y1="' + satHi + '" x2="' + W + '" y2="' + satHi +
      '" stroke="#ffb020" stroke-opacity=".35" stroke-dasharray="4 3"/>' +
      '<line x1="0" y1="' + satLo + '" x2="' + W + '" y2="' + satLo +
      '" stroke="#ffb020" stroke-opacity=".35" stroke-dasharray="4 3"/>' +
      '<polyline fill="none" stroke="#4da3ff" stroke-width="1.6" points="' + pts + '"/>';
    $("d-chart").innerHTML = svg;
    $("d-count").textContent = samples.length + " mesures · " +
      vals.length + " points";
  }

  function renderQuality(samples) {
    var cls = ["ok", "stale", "sat", "fault", "uncertain"];
    var html = samples.map(function (s) {
      return '<i class="' + cls[s.q] + '" title="seq ' + s.seq + " : " +
        QNAME[s.q] + '"></i>';
    }).join("");
    $("d-quality").innerHTML = html;
  }

  function renderLatency(ages) {
    var sorted = ages.slice().sort(function (a, b) { return a - b; });
    var n = sorted.length;
    var mean = n ? Math.round(sorted.reduce(function (a, b) { return a + b; }, 0) / n) : 0;
    var rows = [
      ["échantillons", n],
      ["min", sorted[0] || 0],
      ["p50", percentile(sorted, 50)],
      ["p99", percentile(sorted, 99)],
      ["p99.9", percentile(sorted, 99.9)],
      ["max", sorted[n - 1] || 0],
      ["moyenne", mean]
    ];
    $("d-latency").innerHTML = rows.map(function (r) {
      return '<div class="stat"><div class="k">' + r[0] + '</div><div class="v">' +
        r[1] + '</div></div>';
    }).join("");
  }

  function renderJournal(res) {
    $("d-journal").textContent = res.journal.split("\n").slice(0, 6).join("\n") +
      (res.journal.split("\n").length > 6 ? "\n…" : "");
  }

  var lastFp = null;
  function renderFingerprint(res) {
    $("d-fp").textContent = res.fp;
    var v = $("d-fp-verdict");
    if (lastFp === null) {
      v.textContent = " (première exécution)";
      v.className = "hint";
    } else if (lastFp === res.fp) {
      v.textContent = " ✓ identique — rejeu déterministe";
      v.className = "hint";
      v.style.color = "var(--ok)";
    } else {
      v.textContent = " ✗ différent — paramètres changés";
      v.className = "hint";
      v.style.color = "var(--sat)";
    }
    lastFp = res.fp;
  }

  /* ----------------------------- câblage ----------------------------- */
  function readCfg() {
    var deg = 0;
    var coef = [0n, 0n, 0n, 0n];
    if ($("d-calib").value === "poly") {
      coef = [BigInt($("d-c0").value || 0), BigInt($("d-c1").value || 0),
              BigInt($("d-c2").value || 0), BigInt($("d-c3").value || 0)];
      for (deg = 3; deg > 0 && coef[deg] === 0n; deg--) { /* deg = dernier non nul */ }
    }
    return {
      name: $("d-name").value || "demo",
      wf: parseInt($("d-wf").value, 10),
      seed: parseInt($("d-seed").value, 10) || 0,
      base: parseInt($("d-base").value, 10) || 0,
      amp: parseInt($("d-amp").value, 10) || 0,
      period: parseInt($("d-period").value, 10) || 1,
      drift: parseInt($("d-drift").value, 10) || 0,
      satMin: parseInt($("d-satmin").value, 10),
      satMax: parseInt($("d-satmax").value, 10),
      steps: Math.max(1, parseInt($("d-steps").value, 10) || 1),
      calibKind: $("d-calib").value,
      coef: coef, deg: deg,
      latency: parseInt($("d-lat").value, 10) || 0,
      jitter: parseInt($("d-jit").value, 10) || 0,
      staleMs: parseInt($("d-stale").value, 10) || 0
    };
  }

  function run() {
    var cfg = readCfg();
    var res = simulate(cfg);
    renderChart(res.samples, cfg);
    renderQuality(res.samples);
    renderLatency(res.ages);
    renderJournal(res);
    renderFingerprint(res);
  }

  function init() {
    var ids = ["d-name", "d-wf", "d-seed", "d-base", "d-amp", "d-period", "d-drift",
      "d-satmin", "d-satmax", "d-steps", "d-calib", "d-c0", "d-c1", "d-c2",
      "d-c3", "d-lat", "d-jit", "d-stale"];
    ids.forEach(function (id) {
      $(id).addEventListener("change", run);
      if ($(id).type === "number") $(id).addEventListener("input", run);
    });
    $("d-run").addEventListener("click", run);
    $("d-again").addEventListener("click", run);
    $("d-preset").addEventListener("click", function () {
      $("d-name").value = "temp-chassis";
      $("d-wf").value = "1"; $("d-seed").value = 42; $("d-base").value = 42500;
      $("d-amp").value = 41250; $("d-period").value = 64; $("d-drift").value = 0;
      $("d-satmin").value = -40000; $("d-satmax").value = 125000;
      $("d-steps").value = 180; $("d-calib").value = "poly";
      $("d-c0").value = 0; $("d-c1").value = 1000000; $("d-c2").value = 0;
      $("d-c3").value = 0; $("d-lat").value = 1500000; $("d-jit").value = 0;
      $("d-stale").value = 500; lastFp = null; run();
      $("d-preset-note").innerHTML =
        "Paramètres de <code>core/tests/valid/temp-chassis.json</code> " +
        "(base 42500, amp 41250, calib poly a1=1.0). Première mesure : " +
        "<b>42500</b> — identique à la sortie réelle de <code>glaned-sim</code>.";
    });
    $("d-reset").addEventListener("click", function () {
      $("d-name").value = "demo";
      $("d-wf").value = "1"; $("d-seed").value = 42; $("d-base").value = 20000;
      $("d-amp").value = 10000; $("d-period").value = 64; $("d-drift").value = 0;
      $("d-satmin").value = -40000; $("d-satmax").value = 125000;
      $("d-steps").value = 180; $("d-calib").value = "none";
      $("d-c0").value = 0; $("d-c1").value = 1000000; $("d-c2").value = 0;
      $("d-c3").value = 0; $("d-lat").value = 1500000; $("d-jit").value = 0;
      $("d-stale").value = 500; lastFp = null; $("d-preset-note").textContent = ""; run();
    });
    run();
  }

  window.GLANE_DEMO = { simulate: simulate, calibApply: calibApply, wave: wave,
                        hashNoise: hashNoise, percentile: percentile, fnv1a: fnv1a };

  if (document.readyState === "loading") {
    document.addEventListener("DOMContentLoaded", init);
  } else {
    init();
  }
})();
