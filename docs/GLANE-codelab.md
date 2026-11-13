# Glane codelab — publish a sensor in four chapters

A beginner-friendly path from an empty terminal to a sensor published in a real
QNX 8.0 guest. Each chapter ends with a checkpoint: if you can say the answer
out loud, you understood the chapter.

Every command in this codelab was run on this repository and produced the
output shown. Nothing here is a picture of a picture.

- **Prerequisite:** a Linux machine with `gcc`, `make`, Python 3 (standard
  library only) and `bash`. For chapter 3 you also need the QNX SDP 8.0
  (free, non-commercial — see [`qnx/README.md`](../qnx/README.md)).
- **Time:** ~15 minutes for chapters 1–2, ~15 more for chapter 3, ~10 to read
  chapter 4.

---

## Chapter 1 — Describe a sensor

In Glane, a sensor *is* a description file. Before touching any code, write
one.

### 1.1 Look at a real description

```sh
cat core/tests/valid/temp-chassis.json
```

```json
{"id": 1, "name": "temp-chassis", "driver": "tmp",
 "bus": {"kind": "i2c", "dev": "/dev/i2c0", "addr": "0x48"},
 "period_ms": 100, "unit": "C", "range": [-40.0, 125.0],
 "calib": {"kind": "poly", "coef": [0.0, 1.0]},
 "thresholds": {"warn": 85.0, "crit": 105.0},
 "deadline_ms": 150, "stale_after_ms": 500}
```

Read it as a contract, field by field:

| Field | Meaning | Why it is there |
|---|---|---|
| `driver` | **required** — the name of the driver that reads the bus | the publisher must know *who* talks to the sensor |
| `bus` | where the measurement comes from (`i2c`, `spi`, `gpio`, `file`, `virtuel`) | the transport to talk to |
| `period_ms` | how often the sensor produces a value | each sensor has its own cadence |
| `range` | physical bounds | out-of-range is a fault, not a silent value |
| `calib` | a polynomial `a0 + a1·x + a2·x² + …` applied to the raw value | raw ADC counts are meaningless without it |
| `thresholds` | warn / crit levels | the subscriber (chapter 3) reacts to these |
| `deadline_ms` / `stale_after_ms` | freshness rules | a value becomes STALE or FAULT when the source stops talking |

### 1.2 Your first description

Write your own sensor — a battery voltage, in volts, sampled every 200 ms:

```sh
cat > /tmp/batt-soc.json <<'EOF'
{"id": 2, "name": "batt-soc", "driver": "tmp", "unit": "V",
 "bus": {"kind": "virtuel"},
 "period_ms": 200, "range": [2.5, 4.2],
 "calib": {"kind": "poly", "coef": [0.0, 1.0]},
 "thresholds": {"warn": 3.4, "crit": 4.2},
 "deadline_ms": 300, "stale_after_ms": 1000}
EOF
```

`"bus": {"kind": "virtuel"}` is the *only* generator implemented end-to-end in
this repository. That is perfectly fine for chapters 1–3; chapter 4 explains
exactly what changes for a real bus.

### 1.3 Run it, then break it on purpose

```sh
make all >/dev/null
./build/glaned-sim /tmp/batt-soc.json 20 const 1 1000000 /tmp/batt.jsonl
head -n 1 /tmp/batt.jsonl
```

```json
{"s":"batt-soc","id":2,"seq":1,"ts_ns":0,"d_ns":1000000,"v_m":3350,"u":"V","q":"ok","f":"first|calibrated|simulated"}
```

Now make the description *wrong* — here the warning threshold is above the
critical one, which the validator refuses:

```sh
sed 's/"warn": 3.4, "crit": 4.2/"warn": 4.2, "crit": 3.4/' /tmp/batt-soc.json > /tmp/broken.json
./build/glaned-sim /tmp/broken.json 20 const 1 1000000 /tmp/y.jsonl
```

```text
description invalide: champ=thresholds ligne=7: warn doit être <= crit
```

The tool refuses it, names the field, and gives the line. Remove the
`driver` field and it tells you the same thing about a different field:

```text
description invalide: champ=driver ligne=7: champ obligatoire manquant
```

**Checkpoint:** without re-reading the file, say what happens when a value
leaves its `range`, and what `deadline_ms` has to do with `stale_after_ms`.

---

## Chapter 2 — Publish it virtually and measure it

The host publisher is a perfect twin of the QNX one: same core, same
calibration, same journal. Run it and watch the numbers become evidence.

### 2.1 Run the virtual publisher

```sh
./build/glaned-sim core/tests/valid/temp-chassis.json 2000 sine 42 1500000 /tmp/j1.jsonl
head -n 3 /tmp/j1.jsonl
```

Each line is one measurement, timestamped at the source:

```json
{"s":"temp-chassis","id":1,"seq":1,"ts_ns":0,"d_ns":1500000,"v_m":42500,"u":"C","q":"ok","f":"first|calibrated|simulated"}
{"s":"temp-chassis","id":1,"seq":2,"ts_ns":100000000,"d_ns":101500000,"v_m":46543,"u":"C","q":"ok","f":"calibrated|simulated"}
{"s":"temp-chassis","id":1,"seq":3,"ts_ns":200000000,"d_ns":201500000,"v_m":50548,"u":"C","q":"ok","f":"calibrated|simulated"}
```

`v_m` is the value in **milli-units** (42 500 milli-°C = 42.5 °C) — integers,
never floats, so the calibration is replayed bit-for-bit by the Python twin.

### 2.2 Two runs, one fingerprint

```sh
./build/glaned-sim core/tests/valid/temp-chassis.json 2000 sine 42 1500000 /tmp/j2.jsonl
cmp /tmp/j1.jsonl /tmp/j2.jsonl && echo "IDENTICAL byte for byte"
```

Determinism is a feature, not a detail: the same description, seed and
waveform must produce the same journal, so a test can never pass by accident.

### 2.3 Measure the publication latency

```sh
python3 host/glat.py /tmp/j1.jsonl      # p50 / p99 / p99.9
python3 host/greport.py /tmp/j1.jsonl /tmp/report.html
```

```json
{
  "count": 2000,
  "max_ns": 1500000,
  "mean_ns": 1500000.0,
  "min_ns": 1500000,
  "p50_ns": 1500000,
  "p99_9_ns": 1500000,
  "p99_ns": 1500000,
  "seq_gaps": {},
  "total_losses": 0
}
```

**Checkpoint:** explain the difference between `ts_ns` (source timestamp) and
`d_ns` (delivery timestamp), and which one `glat.py` uses to compute latency.

---

## Chapter 3 — Publish it inside a real QNX 8.0 guest

Chapters 1–2 are deterministic, but they do not prove the code runs on QNX.
This chapter does: the *same* publisher, inside a real QNX 8.0 guest.

Prerequisite: QNX SDP 8.0 installed (default `~/qnx800`), see
[`qnx/README.md`](../qnx/README.md).

### 3.1 Build for QNX and boot the guest

```sh
bash qnx/run-glane-qemu.sh
```

That one command: compiles with `qcc` (target `gcc_ntox86_64`), assembles a
minimal diskless IFS, boots it with `qemu-system-x86_64 -kernel`, and captures
the serial console into `qnx/capture/`.

### 3.2 What the guest proved

Read the capture:

```sh
cat qnx/capture/runtime-uname.txt    # the guest's OWN uname, not a claim
cat qnx/capture/runtime-console.txt
```

Look for, in order:

1. `[QNX-PROOF] sysname=QNX release=8.0.0 …` — `uname` run *inside* the guest.
2. `pidin` listing `procnto-smp-instr` (pid 1) — a real QNX process table.
3. `glaned: 6 mesures publiées pour 'temp-chassis'` — real publication.
4. `ls -l /dev/shmem` — six leaves per sensor (`value`, `quality`, `seq`,
   `ts_ns`, `unit`, `info`), real files inside the guest.
5. `gwatch[…]=6750 ALARME (seuil=3000)` — a subscriber reading the published
   value and reacting to a threshold.
6. `gsensor_resmgr: ATTACH OK /dev/sensors/temp-chassis` then
   `cat /dev/sensors/temp-chassis -> 61944` — a **QNX resource manager**: the
   sensor is a device node, not a log line.
7. `REJEU-RUN1` / `REJEU-RUN2` — the same description replayed twice *inside
   the guest*; the capture script compares the two blocks byte for byte and
   prints the verdict (`IDENTIQUES octet pour octet (6 mesures comparées …)`).
8. `gsuper: … terminé (reprise #1)` — the publisher was killed, the supervisor
   restarted it, and it is alive again in `pidin`. The supervisor is bounded
   (`-m`), so it can never loop forever.

### 3.3 The live guest (interactive)

The same image also boots with a VGA console and an interactive `ksh` (see
[`qnx/VITRINE.md`](../qnx/VITRINE.md)). In the browser you see the VGA screen
and the serial console, and you can type inside the guest:

```sh
ls /dev/shmem
cat /dev/shmem/temp-chassis.info      # the live value, seq increasing
ls /dev/sensors                        # the resource-manager node
```

**Checkpoint:** how do you know the `glaned` in the capture is the QNX binary
and not a host program dressed up? (Hint: `uname`, `pidin`, and the
`ldqnx-64.so.2` interpreter recorded in `qnx/capture/build-proof.txt`.)

---

## Chapter 4 — A real sensor (and what "real" costs)

Chapters 1–3 use the **`virtuel`** bus. Here is precisely what changes for a
physical sensor, and what is — and is not — implemented today.

### 4.1 The driver contract

A Glane *driver* is the smallest thing that turns "read the transport" into a
raw sample: it takes the sensor description and returns the raw value and a
timestamp. Everything else — calibration, range checks, staleness, publication,
latency measurement — is already done by `core/` and the publisher, and does
not change from one sensor to another.

The description already carries everything a driver needs:

```json
{"bus": {"kind": "i2c", "dev": "/dev/i2c0", "addr": "0x48"}}
```

### 4.2 A worked example — BME280 over I²C on a Raspberry Pi 5

QNX 8.0 ships a Raspberry Pi 5 image (the hackathon's reference hardware). A
BME280 answers on I²C address `0x76`. The driver would:

1. `open("/dev/i2c0")` and read the identification register `0xD0` (must be
   `0x60`).
2. Read the 3-byte raw temperature and apply the BME280 compensation formula.
3. Hand the value to the same calibration / range / staleness path as
   chapter 1.

Because the value ends up in `glane_calib_apply` and `glane_publish_sample`,
the *publication* side is already correct: only the raw read differs.

### 4.3 Honest status: what exists and what does not

This repository implements the **`virtuel` generator only**, end to end. The
parser accepts and validates the `i2c`, `spi`, `gpio`, `file` and `virtuel`
bus kinds, but the `i2c`/`spi`/`gpio`/`file` *readers are not implemented*. No
physical bus transaction has been performed here; every number in every
capture is synthetic. This is stated in the README, in the Devpost text, and
here — no claim is made that a real sensor was read.

What a real-sensor driver needs, in this codebase's shape:

1. A `driver_read(desc, out_sample)` function in `publish/qnx/`, per bus kind.
2. A switch in `target/glaned.c` that calls either the `virtuel` generator or
   the bus reader, according to `desc.bus.kind`.
3. The `i2c`/`spi` open + ioctl sequence (the SDP ships the drivers, not a
   sensor-reader SDK — which is exactly the gap Glane fills on the publication
   side).

The four chapters close the loop: **describe → publish (virtual) → publish
(QNX) → the contract a real sensor plugs into.** A reader who finished chapter
3 can wire chapter 4 without touching the core.

---

## Where to go next

- The frozen contract: [`CONTRACT.md`](../CONTRACT.md) — the ABI, the
  calibration scale, the journal format.
- The 24 requirements: [`docs/GLANE-cahier-des-charges.md`](GLANE-cahier-des-charges.md).
- The traceability matrix: [`docs/GLANE-traceabilite.md`](GLANE-traceabilite.md).
- The 100 ideas: [`docs/GLANE-100-idees-amelioration.md`](GLANE-100-idees-amelioration.md).