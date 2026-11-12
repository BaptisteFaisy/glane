# Glane — matrice de traçabilité GLA

| Exigence | Niv. | Couverte | Tests / fichiers |
|---|---|---|---|
| GLA-01 | S | ✅ | `core/tests/test_util.h`, `host/tests/test_trace.py`, `scripts/glane_trace.py` |
| GLA-02 | S | ✅ | `core/src/desc.c`, `core/src/desc_validate.c`, `core/src/jsonscan.c`, `core/tests/test_desc.c`, `host/tests/test_trace.py` |
| GLA-03 | S | ✅ | `core/src/ring.c`, `core/src/sample.c`, `core/src/virtuel.c`, `core/tests/test_ring.c`, `core/tests/test_sample.c`, `core/tests/test_virtuel.c` |
| GLA-04 | S | ✅ | `core/tests/test_virtuel.c`, `host/tests/test_sim.py`, `host/tests/test_trace.py`, `tools/glaned-sim.c` |
| GLA-05 | S | ✅ | `core/src/calib.c`, `core/tests/test_calib.c`, `host/glane_twin.py`, `host/tests/test_trace.py`, `host/tests/test_twin.py`, `tools/calib_dump.c` |
| GLA-06 | S | ✅ | `core/src/deadline.c`, `core/tests/test_deadline.c` |
| GLA-07 | S | ✅ | `core/src/deadline.c`, `core/tests/test_deadline.c` |
| GLA-08 | S | ✅ | `core/src/schema.c`, `core/tests/test_schema.c`, `host/gschema.py`, `host/tests/test_gschema.py`, `host/tests/test_trace.py` |
| GLA-09 | S | ✅ | `host/glat.py`, `host/tests/test_glat.py` |
| GLA-10 | S | ❌ | _(aucun)_ |
| GLA-11 | S | — | `scripts/sizecheck.py` |
| GLA-12 | C | ❌ | _(aucun)_ |
| GLA-13 | C | ❌ | _(aucun)_ |
| GLA-14 | C | ❌ | _(aucun)_ |
| GLA-15 | C | ❌ | _(aucun)_ |
| GLA-16 | C | ✅ | `core/src/ring.c`, `core/tests/test_ring.c` |
| GLA-17 | C | ❌ | _(aucun)_ |
| GLA-18 | C | ❌ | _(aucun)_ |
| GLA-19 | X | ❌ | _(aucun)_ |
| GLA-20 | X | ❌ | _(aucun)_ |
| GLA-21 | X | ❌ | _(aucun)_ |
| GLA-22 | X | ❌ | _(aucun)_ |
| GLA-23 | X | ❌ | _(aucun)_ |
| GLA-24 | X | ✅ | `host/tests/test_trace.py` |

**11/24 exigences citées dans un test.**
