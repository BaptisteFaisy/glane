# Devpost submission — Glane

> Ready to paste into the Devpost form. English, per §13 of the spec
> (`docs/GLANE-cahier-des-charges.md`).
> Every figure quoted here comes from a file in the repository or from a command
> run on this machine. Nothing is estimated.

**Project name:** Glane — a sensor layer and device-publication layer for QNX 8.0
**Category:** Low-level Software
**Bonus tracks:** Best use of QNX-specific Technologies
**Repository:** <https://github.com/Tradoooo/glane>
**Live demo page:** <https://tradoooo.github.io/glane/>

> **Not a sensor fusion framework, not a field bus.** Glane stops at one thing:
> a sensor described in a file, calibrated, published into the QNX namespace with
> a source timestamp — and the publication latency **measured on the target**.

---

## Inspiration

QNX ships a device-publication framework and exactly **one** publisher with it
(`usblauncher_otg`, aimed at USB, storage and media). There is nothing for
sensors. The guide documents what a published object contains and which
command-line options exist, but there is no API for *writing* a publisher, no
way to simulate a sensor without hardware, no conformance validator for the
fixed-slot object tree the framework imposes, and nothing at all about
publication latency or notification timestamping. The data model is media-shaped
— mountpoints, partitions, tables of contents, disk temperature — so it carries
no units, no ranges, no calibration and no sensor timestamp.

That is the gap this project fills. Not by inventing a new mechanism, but by
supplying the missing pieces: a portable description/calibration/validation
core, a publisher that runs on QNX 8.0, a resource manager that exposes the
published sensors as real device nodes, a subscriber that reacts to them, and a
latency bench that turns "publication is fast" into a number.

## What it does

A sensor is described in a JSON file — bus, period, unit, range, calibration
coefficients, thresholds, deadlines. From that description, Glane produces
measurements, applies the calibration, stamps them at the source, publishes
them into the QNX namespace as a per-sensor object
(`value`, `quality`, `seq`, `ts_ns`, `unit`, `info`), and **measures the
publication latency on the target**.

| Piece | What it is |
|---|---|
| `core/` | Pure C11 core: JSON description parsing and validation, integer calibration, ring buffer, deadline/staleness logic, published-object schema. No clock, no file, no socket, no allocation, no mutable global state — the caller injects everything. |
| `target/glaned` | The multi-sensor publisher for QNX 8.0. Reads several descriptions, generates measurements at each declared period (the virtual generator is the only source implemented today), calibrates, publishes, and reports p50/p99/p99.9/max of the publication latency measured with `ClockTime(CLOCK_MONOTONIC)`. |
| `publish/qnx/` | The publication layer: creates the sensor objects in the target namespace and writes the six leaves. Falls back to a flat layout on filesystems that do not implement `mkdir`, and to stdout if nothing is writable. |
| `target/gsensor_resmgr` | A QNX resource manager (`resmgr` + `iofunc`): attaches `/dev/sensors/<name>`, so a published sensor is opened and read like a device, not like a log line. |
| `target/gwatch` | A subscriber that polls a published sensor and raises an alarm when the value crosses a threshold. |
| `target/gsuper` | A supervisor that restarts the publisher when it dies, and counts and dates every restart. |
| `host/` | Python 3 (standard library only): schema validator for a published tree, latency bench (`p50/p99/p99.9/max/min/mean` + sequence-gap counters), standalone HTML report, canonicaliser, and the Python twin of the integer calibration. |
| `scripts/` | Mutation test of the C core, requirement traceability matrix, size guard. |

The published measurement ABI is frozen: a 24-byte POD with no pointer and no
float — `sensor_id`, monotone `seq`, source timestamp `ts_ns`, integer
`value_milli`, `unit`, `quality` (OK / STALE / SATURATED / FAULT / UNCERTAIN)
and `flags` (origin, not quality). Values are integer milli-units, so the Python
twin can replay the C calibration bit for bit.

## How we built it

**Portable core first, OS second.** `core/` is C11 strict
(`-std=c11 -O2 -Wall -Wextra -Werror`), compiles and tests on Linux, and knows
nothing about QNX: no `malloc`, no file, no socket, no clock, no mutable
global. Everything that touches the world is injected by the caller. 11 source
files, 11 headers, 1 536 lines of implementation, 619 lines of headers.

**One contract, two implementations.** The frozen contract (`CONTRACT.md`)
defines the measurement ABI, the integer calibration scale and its bounds, the
description format, the published-name charset and the journal format. The
Python twin re-implements the calibration with arbitrary-precision integers and
is compared against the C implementation over **10 000 random vectors** drawn
inside the contract bounds with a fixed seed — **zero divergence**.

**The QNX side is isolated and small.** Only `target/glaned.c` and
`publish/qnx/glane_publish.c` touch the OS; everything else compiles unchanged
on the host. The publisher is built for QNX 8.0 with `qcc 12.2.0`, target
variant `gcc_ntox86_64`, and runs inside a real QNX 8.0 guest booted from a
minimal diskless image.

**Everything is measured, never asserted.** The publisher brackets each
publication with two `ClockTime(CLOCK_MONOTONIC)` reads and prints
p50/p99/p99.9/max at the end of the run. The host bench (`host/glat.py`)
computes nearest-rank percentiles, min/mean/max and sequence-gap counters from
a journal.

**Verification actually runs, in CI:**

| Check | Result |
|---|---|
| C unit tests (`make test`) | **973 checks**, 9 binaries, each printing `OK <n> checks` |
| ASan + UBSan pass (`make asan`) | green, same 973 checks |
| Host tests (`make host-test`) | **32 tests**, Python standard library only |
| Differential C ↔ Python calibration | **10 000 values**, fixed seed, **0 divergence** |
| Mutation test | **10/10 mutants killed** |
| Parser fuzz under ASan/UBSan (`make fuzz`) | **100 000 inputs**, no crash, no leak |
| Invalid-description bank | **40 faulty descriptions**, each rejected with its expected error code |
| Size guard | **42 files** under the limits (< 500 lines/file, < 50 lines/function) |

**CI:** two parallel jobs on hosted runners (`build-test`, `host-test`, both
green, `timeout-minutes: 5`, `fetch-depth: 1`, concurrency cancellation), plus a
dedicated **QNX 8.0 job on a self-hosted runner** — the only place where the
SDP is licensed. That job compiles for QNX, builds the image, boots the guest
and captures the console; the latest run took **1m42s**.

**Reproduce it:** `make && make test && make host-test && make asan &&
make sizecheck && make fuzz && python3 scripts/glane_mutate.py`; then, for the
QNX 8.0 target (SDP 8.0, default `~/qnx800`):

```sh
bash qnx/build.sh   # qcc -V gcc_ntox86_64 -std=c11 -O2 -Wall -Wextra
qemu-system-x86_64 -kernel build-qnx/glane-ifs.bin -m 512 -smp 1 -cpu max \
  -display none -vga none -serial file:build-qnx/qnx-console.log -no-reboot
```

## Challenges we ran into

- **The installed SDP 8.0 does not ship the PPS, io-pkt or HAM components.**
  We searched the whole SDP tree (`find ~/qnx800 -iname '*pps*' -o -iname
  '*io-pkt*' -o -iname 'ham*'`) and found none of them. So there is no native
  PPS publication path, and no real HAM to register the publisher with. We said
  so and changed the design instead of faking it: publication goes through a
  **resource manager we wrote** (`target/gsensor_resmgr.c`, `resmgr_attach` +
  `iofunc`), and supervision goes through a small supervisor (`target/gsuper.c`)
  that restarts the publisher and records each restart. The mechanism is the
  same as HAM's — restart a dead publisher — but it is our code, not HAM, and we
  do not claim otherwise. This is the honest fallback the spec planned
  (`CONTRACT.md` §8 leaves the QNX publication API open on purpose).
- **A published sensor is a filesystem, and filesystems differ.** The
  publication layer tries candidate roots in order (`GLANE_SENSOR_DIR`,
  `/dev/shmem/sensors`, `/tmp/sensors`, `/sensors`, `/var/sensors`), and
  detects that a candidate cannot hold a directory tree: the sensor object is
  then written flat (`<name>.value`, `<name>.quality`, …) in the same root. If
  no root is writable at all, the publisher falls back to stdout and says so.
- **Measuring on the target is what actually counts.** On the guest,
  publication latency is dominated by the file writes into the namespace: over
  the recorded runs, p50 stayed between **439 435 ns and 771 060 ns**, and
  the worst p99 reached **4 877 728 ns**. The target number brackets each publish
  call with two `ClockTime(CLOCK_MONOTONIC)` reads; the host bench (`glat`)
  measures delivery latency from the journal (`d_ns` − `ts_ns`), which in the
  host simulator is an injected, fixed value — the two are different measures
  and only the target one is published here.
- **Our own percentile helper has a limit.** On the target, the p99.9 rank is
  computed as `p * n / 100` with `p = 999`, which clamps to the last sample, so
  in the captured runs the `p99.9` column equals `max`. We publish the host
  bench (`host/glat.py`, nearest-rank) as the reference implementation and keep
  the target number for what it is.
- **The startup script of an IFS image is not a shell.** `exec ksh -l` inside
  the `[+script]` block fails with `Unable to start "exec" (2)`. The
  interactive shell had to be launched with `on`, which starts a process on a
  console and backgrounds it.
- **No physical sensor board was available in this environment.** Every
  measurement shown here comes from the deterministic virtual generator inside
  the QNX guest. The description format accepts and validates the `i2c`, `spi`,
  `gpio`, `file` and `virtuel` bus kinds, but only the virtual generator is
  exercised end to end: no physical bus transaction was performed, and we make
  no claim about real hardware.

## Accomplishments we're proud of

- **A real QNX 8.0 guest, proved from inside the guest.** `uname` executed in
  the target returns
  `sysname=QNX release=8.0.0 version=2026/06/05-16:26:03EDT machine=x86pc node=localhost`.
- **Real publication, listed and read.** The publisher created the sensor
  objects in the guest namespace; `ls -l` shows the six leaves per sensor with
  real sizes and timestamps, and their content is the measurement JSON.
- **A device node, not a log line.** The resource manager attached
  `/dev/sensors/temp-chassis` (`ATTACH OK`), the node appears in `ls -l
  /dev/sensors`, and `cat /dev/sensors/temp-chassis` returns the current
  published value (`61944`).
- **A subscriber that reacts.** `gwatch` reads the published value and raises
  `ALARME (seuil=3000)` on every sample above the threshold — the
  publish/subscribe loop closing inside the guest.
- **Supervision that survives a kill.** The publisher was killed in the guest;
  the supervisor restarted it and the console records `reprise #1`, with the new
  process visible again in `pidin`. The supervisor takes an explicit restart
  bound (`gsuper -m N`) so it stops after N restarts instead of looping, and
  the restarted publisher keeps publishing in place.
- **Three sensors at three different rates, published together.** One run
  published **68** measurements — 6 for `temp-chassis` (100 ms), 60 for
  `imu-accel` (10 ms), 2 for `batt-voltage` (500 ms) — with a measured
  publication latency of p50 = **439 435 ns**, p99 = **2 994 900 ns**.
- **Deterministic replay, on the host and on the target.** Two runs of the
  publisher with the same seed produce byte-identical journals.

## What we learned

- **An integer scale is what makes a twin possible.** Carrying values as
  milli-units, with a fixed coefficient scale, let a Python re-implementation
  agree with the C one on every one of 10 000 random vectors. With floats, the
  same test would have turned into a tolerance argument.
- **Measure on the target, or do not quote the number.** The same publication
  code behaves differently inside the guest. The only figure worth publishing
  is the one the guest printed.
- **Freezing the contract early is what allowed parallel work.** With the ABI,
  the calibration scale, the description format and the journal format fixed,
  the C core, the Python twin, the publisher, the resource manager and the
  tools could be built at the same time without guessing at each other.
- **Discovery, calibration and conformance are the actual gaps in a sensor
  stack.** Publication is the easy half; what a sensor product needs on top is
  knowing which sensor is present, whether its numbers are in range, and
  whether the object published respects the tree it must live in.
- **Honesty is cheaper than a workaround.** Saying "the SDP has no PPS and no
  HAM, so here is a resource manager and a supervisor we wrote" cost one
  paragraph and earned a project that still runs on QNX 8.0.

## What's next for Glane

- **The four-chapter codelab ships** ([`docs/GLANE-codelab.md`](GLANE-codelab.md)) —
  describe a sensor, publish it, publish it on QNX, then the driver contract for
  a real sensor. Next: a `glane lab` command that boots a throwaway guest and
  checks each chapter automatically.
- **HAM integration** for the publisher once an SDP that ships it is
  available, with the measured restart time replacing the hand-written
  supervisor.
- **Native PPS export** of the measurements, so existing QNX subscribers can
  consume them without our resource manager.
- **Real sensor boards**, to close the gap between the virtual generator and
  physical `i2c`/`spi` transactions, and the source-timestamp accuracy question
  that comes with them.
- **A security policy** for the published objects, including the command
  object, checked by a refusal test.
- **Automatic bus discovery**, per-sensor quality-of-service budgets with
  declared degradation under bus saturation, and hardware timestamping with a
  measured comparison against the software timestamp.

---

### Demo script for the video

See [`docs/VIDEO.md`](VIDEO.md) — second-by-second script, with what is on
screen and what is said.

### Sources — where each figure comes from

| Figure | Source in the repository |
|---|---|
| 24 requirements, 11 S / 7 C / 6 X | `docs/GLANE-cahier-des-charges.md` §6 |
| Frozen ABI, calibration scale, description format, journal format | `CONTRACT.md` §1–§7 |
| 973 C checks, 32 host tests, 10 000-value differential, 10/10 mutants, 40 faulty descriptions, size guard, fuzz count | `make test`, `make host-test`, `make asan`, `make fuzz`, `scripts/glane_mutate.py`, `scripts/sizecheck.py`, `core/tests/faulty/` (40 files) — all green on this machine |
| Guest OS proof (`sysname=QNX release=8.0.0 …`) | `qnx/capture/runtime-uname.txt` |
| Toolchain (`qcc 12.2.0`, `gcc_ntox86_64`), compile command, zero warning | `qnx/capture/build-proof.txt`, `qnx/build.sh` |
| Boot command, image build, replay instructions | `qnx/capture/runtime-howto.md`, `qnx/glane-ifs.build` |
| 3 sensors / 68 measurements, p50 = 439 435 ns, p99 = 2 994 900 ns, published objects, `gwatch` alarm, `resmgr` attach, `cat /dev/sensors/…` → 61944, restart sequence `reprise #1` | `qnx/capture/runtime-glaned.txt`, `qnx/capture/runtime-console.txt` |
| p50 range 439 435–771 060 ns, p99 up to 4 877 728 ns | `qnx/capture/runtime-console.txt` (the three latency reports of the capture) |
| Deterministic replay on the target (`REJEU-RUN1` / `REJEU-RUN2` compared byte for byte) | `qnx/capture/runtime-console.txt`, `qnx/capture/runtime-howto.md`, `qnx/run-glane-qemu.sh` |
| Codelab of 4 chapters | `docs/GLANE-codelab.md` |
| Publication roots, flat-layout fallback, stdout fallback | `publish/qnx/glane_publish.c` |
| Resource manager, subscriber, supervisor behaviour | `target/gsensor_resmgr.c`, `target/gwatch.c`, `target/gsuper.c` |
| Host bench, schema validator, twin | `host/glat.py`, `host/gschema.py`, `host/glane_twin.py` |
| CI jobs, self-hosted QNX runner, latest QNX run 1m42s | `.github/workflows/ci.yml`, `.github/workflows/qnx-build.yml`, GitHub Actions run history |

<!-- Codelab URL (§13.6 of the spec): the codelab now exists at
     docs/GLANE-codelab.md — use
     https://github.com/Tradoooo/glane/blob/main/docs/GLANE-codelab.md in the
     Devpost field. -->
<!-- IFS size: resolved — qnx/run-glane-qemu.sh now measures the built image
     instead of a hard-coded size in qnx/capture/runtime-howto.md. -->
