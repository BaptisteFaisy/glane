# Glane — 3-minute demo video script

**Duration: exactly 3:00 (180 s).** Language: English (narration and on-screen
text). No music, no third-party logo on screen, no trademark other than the QNX
console output that the project is about.

**Everything shown on camera is real output.** Each shot names the exact
command to type and the capture file that holds the output to show. Nothing in
this script claims a Raspberry Pi 5 or a physical sensor: the operating system
is a **real QNX 8.0.0 guest** under QEMU, the sensors are **synthetic
waveforms** produced by the Glane virtual publisher.

---

## 1. Shot list

| Time | Shot | What is on screen | Narration (English) |
|---|---|---|---|
| 0:00–0:20 | **The gap** | The five-gap table from `README.en.md`, with the QNX documentation page names in the second column | "QNX ships a device-publishing framework — and exactly one publisher: `usblauncher_otg`, for USB, storage and media. Nothing for sensors. No documented API to write a publisher. No simulation without hardware. No conformance validator. No latency measurement. Glane does not replace the framework. It supplies the five missing pieces." |
| 0:20–0:50 | **Host tools** | Terminal: `gschema` on a published tree, then the same tree with one leaf removed; `glat` printing p50/p99/p99.9; `gcanon` printing a SHA-256 | "Before the target: the tools that make the claim checkable. `gschema` validates a published tree against the contract — it returns SCHEMA OK, and SCHEMA FAIL with a non-zero exit the moment one mandatory leaf is missing. `glat` turns a journal into latency percentiles. `gcanon` canonicalises a sensor description and fingerprints it with SHA-256, so a configuration change is a diffable event." |
| 0:50–1:00 | **Proof of OS** | `[QNX-PROOF] sysname=QNX release=8.0.0 …` then `uname -a` | "Now the same code inside a real guest. This line was produced inside the guest by `uname(2)` — not on the host: QNX 8.0.0, x86_64." |
| 1:00–1:08 | **Process table** | `pidin` output: `procnto-smp-instr` pid 1 | "The guest's own process table. `procnto-smp-instr` is PID 1: this is QNX, not a Linux userland." |
| 1:08–1:22 | **Three sensors, three rates** | `glaned: 6 mesures publiées pour 'temp-chassis'` / `60 … 'imu-accel'` / `2 … 'batt-voltage'`, then `ls -l /dev/shmem` listing the published objects | "One publisher, three sensors, three different periods: temperature at 100 ms, IMU at 10 ms, battery at 500 ms. The six mandatory leaves — value, quality, seq, timestamp, unit, info — exist as real files in the guest's namespace." |
| 1:22–1:32 | **Timestamped measurements** | Journal lines from `imu-accel.info` | "Every measurement carries a monotonic timestamp taken at the source, a monotone sequence number, and a quality field. The sequence number is what lets a subscriber detect a loss." |
| 1:32–1:42 | **Latency on the target** | `glane-latence: 68 publications, p50=537353 ns, p99=3759187 ns, p99.9=3759187 ns, max=3759187 ns` | "Latency is measured, not asserted: source timestamp to delivery, on the target, p50, p99 and p99.9. Every published number in this project comes out of a bench like this one." |
| 1:42–1:52 | **Resource manager** | `ATTACH OK /dev/sensors/temp-chassis`, then `cat /dev/sensors/temp-chassis -> 61944` | "A QNX resource manager exposes the sensor as a node in `/dev/sensors`. Any client opens it and reads the current value — no bespoke client needed." |
| 1:52–2:02 | **Subscriber reacts** | `gwatch[/dev/shmem/imu-accel.value] value=6750  ALARME (seuil=3000)` | "And a subscriber that consumes the published value and raises an alarm when it crosses its threshold. Publish, subscribe, react — all inside the guest." |
| 2:02–2:10 | **The publisher is killed and comes back** | `gsuper: '/proc/boot/glaned' terminé (reprise #1)`, then `pidin` showing `glaned` alive again | "We kill the publisher. The supervisor restarts it and counts the recovery. A publisher that dies is not a silent loss of data." |
| 2:10–2:18 | **C tests** | `make test` — nine binaries, `OK <n> checks` each, `ALL TESTS PASSED` | "973 checks across nine C binaries, compiled with `-std=c11 -O2 -Wall -Wextra -Werror`, and the same suite green under AddressSanitizer and UndefinedBehaviorSanitizer." |
| 2:18–2:24 | **Differential test** | `make host-test` — `Ran 32 tests … OK` | "Thirty-two host tests, including a differential test of the integer calibration against an independent Python implementation: ten thousand values, fixed seed, zero divergence." |
| 2:24–2:30 | **Mutation test** | `python3 scripts/glane_mutate.py` — `10/10 mutants tués` | "Mutation testing: ten deliberate bugs injected into the C core — a threshold moved, a comparison inverted — and ten of ten killed by the test suite." |
| 2:30–2:35 | **Fuzz** | `make fuzz` — `OK 100000 entrées fuzzées (seed 11400714819323198485)` | "The description parser is fuzzed under the sanitizers: one hundred thousand deterministic inputs, no crash, no leak." |
| 2:35–2:40 | **CI** | The GitHub Actions page, green, both jobs under five minutes | "And the same thing runs in CI, two jobs, a five-minute budget per job." |
| 2:40–3:00 | **What QNX brings / call to action** | Card: cadence, priorities, source timestamping, resource manager, per-object security model — then the repository, the live demo and the live guest links | "QNX gives the parts that are expensive to rebuild: scheduled sampling with priorities, a resource-manager namespace every client can open, monotonic source timestamping, and a per-object security model. Glane adds what the Device Publishers guide never had: a sensor description format, integer calibration, publication into the namespace, a conformance validator and a latency bench. What you saw ran in a real QNX 8.0.0 guest under QEMU; the sensors were synthetic. Repository: Tradoooo slash glane. Live demo and live QNX guest: the links on screen." |

---

## 2. Commands and capture files, shot by shot

### 0:00–0:20 — The gap

- **On screen:** the five-gap table, second column = the QNX documentation pages
  read for this project (`dev_pub/about.html`, `dev_pub/overview.html`,
  `dev_pub/rmps_objects.html`, `dev_pub/usblauncher_otg.html`).
- **How to display it:**

  ```sh
  sed -n '/^| # | Gap/,/^$/p' README.en.md
  ```

- **Source of the claim:** `docs/GLANE-cahier-des-charges.md` §2.1 and §2.2
  (gap table with the page that documents each gap), status of the claims in
  §14.
- **No capture file:** this shot is a slide over the repository's own README.

### 0:20–0:50 — Host tools

All three commands are run live on the host during the shoot; they take under
a second each.

```sh
# 1. build a tree that follows the published layout (CONTRACT.md §6)
mkdir -p /tmp/tree/temp-chassis
for l in value quality seq ts_ns unit info; do echo x > /tmp/tree/temp-chassis/$l; done

# 2. the validator accepts it
python3 host/gschema.py /tmp/tree            # -> SCHEMA OK   (exit 0)

# 3. remove one mandatory leaf: the validator refuses, with a non-zero exit
rm /tmp/tree/temp-chassis/quality
python3 host/gschema.py /tmp/tree            # -> SCHEMA FAIL (1 écart(s))
                                            #    capteur 'temp-chassis': feuilles obligatoires manquantes ['quality']
echo $?                                      # -> 1

# 4. latency percentiles from a journal
./build/glaned-sim core/tests/valid/temp-chassis.json 2000 sine 42 1500000 /tmp/j.jsonl
python3 host/glat.py /tmp/j.jsonl            # -> p50_ns / p99_ns / p99_9_ns / max_ns / seq_gaps

# 5. SHA-256 fingerprint of a sensor description
python3 host/gcanon.py core/tests/valid/temp-chassis.json --finger
# -> 97d1d248e80292ad8b22cd03996baf40bcea6f349faabc57a93d2e1374fef131
```

- **Output verified on this machine:** `SCHEMA OK` / exit 0; `SCHEMA FAIL
  (1 écart(s))` / exit 1; `glat` prints `count`, `max_ns`, `mean_ns`, `min_ns`,
  `p50_ns`, `p99_9_ns`, `p99_ns`, `seq_gaps`, `total_losses`; the fingerprint
  above.
- **Related committed artefact:** `qnx/capture/runtime-journal.jsonl` holds the
  same journal format, produced by the publisher inside the guest.

### 0:50–2:10 — The QNX 8.0 guest

The guest is booted by `qnx/run-glane-qemu.sh`, whose startup script
(`qnx/glane-ifs.build`) runs the scene. Every frame of this section is a line
of `qnx/capture/runtime-console.txt` (and `qnx/capture/runtime-glaned.txt` for
the publication part).

| Shot | Command run in the guest (from `qnx/glane-ifs.build`) | Capture file and lines | Output to show |
|---|---|---|---|
| Proof of OS | `qnxproof "GLANE sur QNX 8.0 - emulateur qemu-system-x86_64"` then `uname -a` (lines 26 and 30) | `qnx/capture/runtime-uname.txt` (1 line) and `qnx/capture/runtime-console.txt` lines 6 and 11 | `[QNX-PROOF] sysname=QNX release=8.0.0 version=2026/06/05-16:26:03EDT machine=x86pc node=localhost` / `QNX localhost 8.0.0 2026/06/05-16:26:03EDT x86pc x86_64` |
| Process table | `pidin -F "%n %a"` (line 32) | `qnx/capture/runtime-console.txt` lines 12–15 | `name` / `pid` header, `/proc/boot/procnto-smp-instr  1`, `proc/boot/pidin  24578` |
| Three sensors, three rates | `GLANE_SENSOR_DIR=/dev/shmem glaned 60 /proc/boot/temp-chassis.json /proc/boot/imu-accel.json /proc/boot/batt-voltage.json` (line 35) then `ls -l /dev/shmem` (line 38) | `qnx/capture/runtime-glaned.txt` lines 4–6 and 9–28 | `glaned: 6 mesures publiées pour 'temp-chassis'` / `60 … 'imu-accel'` / `2 … 'batt-voltage'`, then the 18 published files (`*.value`, `*.quality`, `*.seq`, `*.ts_ns`, `*.unit`, `*.info` for the three sensors) |
| Timestamped measurements | `tail -n 5 /dev/shmem/imu-accel.info` (line 40) | `qnx/capture/runtime-glaned.txt` lines 29–34 (= `qnx/capture/runtime-journal.jsonl`) | `{"s":"imu-accel","id":2,"seq":56,"ts_ns":550000000,"v_m":5750,"unit":"g","quality":"ok"}` and the four next lines |
| Latency on the target | printed by `glaned` itself (`target/glaned.c`, `glane-latency:` line) | `qnx/capture/runtime-glaned.txt` line 7 | `glane-latence: 68 publications, p50=537353 ns, p99=3759187 ns, p99.9=3759187 ns, max=3759187 ns` |
| Resource manager | `gsensor_resmgr temp-chassis /dev/shmem/temp-chassis.value` (line 45), `ls -l /dev/sensors` (47), `cat /dev/sensors/temp-chassis` (49) | `qnx/capture/runtime-glaned.txt` lines 43–48 | `ATTACH OK /dev/sensors/temp-chassis (depuis /dev/shmem/temp-chassis.value)`, `/dev/sensors/temp-chassis (pid 77827)`, `cat /dev/sensors/temp-chassis -> 61944` |
| Subscriber reacts | `gwatch /dev/shmem/imu-accel.value 3000 6` (line 42) | `qnx/capture/runtime-glaned.txt` lines 35–41 | `gwatch[/dev/shmem/imu-accel.value] value=6750  ALARME (seuil=3000)` (6 lines) |
| Kill and restart | `gsuper /proc/boot/glaned 99999 /proc/boot/temp-chassis.json` (line 52), `slay glaned` (line 54), `pidin -F "%n"` (line 57) | `qnx/capture/runtime-glaned.txt` lines 50–61 | `gsuper: surveille '/proc/boot/glaned' (pid superviseur 106500)`, `gsuper: '/proc/boot/glaned' terminé (reprise #1)`, then `pidin` listing `/proc/boot/glaned` alive again |

Two remarks to keep the video honest:

- The prefixes `gsengsensor_resmgr:` and `sor_resmgr:` in the captured console
  are garbled characters in the recorded text; the binary prints
  `gsensor_resmgr:` (`target/gsensor_resmgr.c`, lines 120 and 152). Either show
  the line without the prefix, or say "the prefix is garbled in the capture".
- In the same capture, the startup script's final `exec ksh -l` reports
  `Unable to start "exec" (2)`: the interactive shell is provided by the
  vitrine image (`qnx/glane-vitrine.build` + `qnx/invite.sh`, see
  `qnx/VITRINE.md`), not by the proof image. If the vitrine is up during the
  shoot, insert 8 s at 2:02 showing the live VGA console
  (`https://baptiste-ms-7d18.tail3a8bdf.ts.net:10000/screen.png`, banner
  `QNX 8.0 - INVITE GLANE` and the `glane>` prompt) and drop the "kill and
  restart" shot to 6 s. <!-- TODO: decide at shoot time whether the vitrine is
  running; the screen capture is a live endpoint, not a committed file -->

### 2:10–2:40 — Proofs

```sh
make test            # 9 binaries: OK 63 / 26 / 171 / 28 / 287 / 35 / 47 / 8 / 308 checks
                     # -> "ALL TESTS PASSED"  (973 checks in total)
make host-test       # "Ran 32 tests in 0.070s" -> "OK"
python3 scripts/glane_mutate.py   # "10/10 mutants tués" / "MUTATION OK"
make fuzz            # "OK 100000 entrées fuzzées (seed 11400714819323198485)"
python3 scripts/sizecheck.py      # "SIZECHECK OK : 42 fichier(s) sous les bornes"
```

- The differential test (10 000 values, zero divergence) is
  `host/tests/test_twin.py::test_10000_values`, run by `make host-test`.
- The CI shot: <https://github.com/Tradoooo/glane/actions> — two jobs
  (`build-test`, `host-test`), each `timeout-minutes: 5`, `fetch-depth: 1`
  (`.github/workflows/ci.yml`).
- All of these outputs were reproduced on this machine on 2026-11-13.

### 2:40–3:00 — What QNX brings / call to action

- **On screen (card 1, 12 s):** `cadence and priorities` · `monotonic source
  timestamping` · `resource-manager nodes in /dev/sensors` · `per-object
  security model`.
- **On screen (card 2, 8 s):** `github.com/Tradoooo/glane` ·
  `tradoooo.github.io/glane` · `baptiste-ms-7d18.tail3a8bdf.ts.net:10000` ·
  `Devpost submission: 15 December 2026, 20:00 Paris time`.
- **Do not claim** a shipped security policy, a real sensor, or a Raspberry Pi
  5: the security policy file is still a template (`image/mkifs.fragment` is
  marked "à valider"), and the sensors are synthetic.

---

## 3. How to record it

### 3.1 Prerequisites

| Item | Status on this machine |
|---|---|
| QNX SDP 8.0 (`~/qnx800`) | required to re-boot the guest; the committed captures let you shoot without it |
| `ffmpeg` | 7.0.2 (`~/bin/ffmpeg`) — `drawtext` is **not** compiled in, so use the PNG-frame renderer below |
| Python 3 + Pillow | 3.12.3 / Pillow 10.2.0 — used to turn a text capture into video frames |
| X display for the live shots | `:0.0`, 1920x1080 (`xrandr`) |
| `script` (util-linux) | available, for a terminal recording of the host segment |

### 3.2 Reproduce the QNX capture (if the SDP is installed)

```sh
cd ~/Bureau/glane
bash qnx/run-glane-qemu.sh     # compiles with qcc, builds the IFS, boots QEMU, captures
```

It writes, in `qnx/capture/`:

| File | Content |
|---|---|
| `runtime-console.txt` | the full serial console of the guest (the whole 0:50–2:10 section) |
| `runtime-glaned.txt` | the publication part of the console (sensors, objects, latency, resmgr, subscriber, supervisor) |
| `runtime-uname.txt` | the one-line proof of OS (`[QNX-PROOF] …`) |
| `runtime-journal.jsonl` | the journal produced by `glaned` inside the guest |
| `runtime-howto.md` | the exact QEMU command line, the host, the toolchain version |
| `build-proof.txt` | the QNX 8.0 compilation proof (`qcc 12.2.0`, `gcc_ntox86_64`, exit 0, no warning) |

Check the live vitrine (the interactive guest exposed on the web):

```sh
bash qnx/recette/recette-glane.sh     # 12 checks: banner, freshness, VGA capture, key injection, funnel, site
```

### 3.3 Record the live segments

- **Host segment (0:20–0:50, 0:10–2:40 proofs):** record the screen with
  `ffmpeg` and `x11grab`, one file per segment:

  ```sh
  ffmpeg -y -f x11grab -framerate 30 -video_size 1920x1080 -i :0.0 -t 30 \
    -c:v libx264 -preset veryfast -pix_fmt yuv420p /tmp/seg-host.mp4
  ```

  (the `x11grab` path was probed on this machine: a 320x240 / 1 s test grab
  succeeded, the probe file was deleted). Alternatively `script -q -c "<cmd>"
  /tmp/seg.typescript` records the terminal as text, which the frame renderer
  of §3.4 then turns into video.

- **QNX segment (0:50–2:10):** either boot the guest live and record the
  serial console, or use the committed capture — the content is identical
  (real output either way), which is what the fallback plan covers.

### 3.4 Assemble the MP4 from the captures (tested)

The QNX console capture is plain text, so it is rendered to PNG frames and
encoded. **Both commands below were run on this machine on 2026-11-13**
(ffmpeg 7.0.2, Pillow 10.2.0) and produce a valid 1280x720 H.264 file.

```sh
cat > /tmp/glane_frames.py <<'EOF'
#!/usr/bin/env python3
"""Rend une capture texte QNX en images PNG (une par frame) pour ffmpeg."""
import sys
from PIL import Image, ImageDraw, ImageFont

src, outdir, dur = sys.argv[1], sys.argv[2], float(sys.argv[3])
fps, W, H = 25, 1280, 720
lines = open(src, encoding="utf-8", errors="replace").read().splitlines()
font = ImageFont.truetype("/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf", 15)
lh, visible = 20, 33
for i in range(int(dur * fps)):
    img = Image.new("RGB", (W, H), (13, 17, 23))
    d = ImageDraw.Draw(img)
    top = int(i / fps * 6)            # lignes défilées par seconde
    for k in range(visible):
        idx = top + k
        if idx >= len(lines):
            break
        d.text((24, 30 + k * lh), lines[idx], font=font, fill=(216, 216, 216))
    img.save(f"{outdir}/{i:05d}.png")
EOF

mkdir -p /tmp/glane_frames
python3 /tmp/glane_frames.py qnx/capture/runtime-console.txt /tmp/glane_frames 80
ffmpeg -y -framerate 25 -i /tmp/glane_frames/%05d.png \
  -c:v libx264 -preset veryfast -pix_fmt yuv420p -r 25 /tmp/seg-console.mp4
```

Then concatenate the segments (tested: two 4 s clips → an 8.000 s MP4):

```sh
printf "file '/tmp/seg-console.mp4'\nfile '/tmp/seg-host.mp4'\n" > /tmp/glane-list.txt
ffmpeg -y -f concat -safe 0 -i /tmp/glane-list.txt -c copy /tmp/glane-3min.mp4
```

Check the result before uploading:

```sh
ffprobe -v error -show_entries format=duration -show_entries stream=width,height,codec_name \
  -of default=noprint_wrappers=1 /tmp/glane-3min.mp4     # duration must be 180 s
```

<!-- TODO: the two segments above are 4 s each because that is what was tested;
the 80 s / 30 s versions use the same commands with different -t values -->

---

## 4. Fallback plan

**If the guest does not boot (no SDP, no QEMU, an IFS build failure):** use the
committed captures. They are not mock-ups — `qnx/capture/runtime-console.txt`
is the raw serial console of a real QNX 8.0.0 guest, recorded on 2026-11-12,
and `qnx/capture/runtime-howto.md` gives the exact command line that produced
it. The 0:50–2:10 section is then narrated over the rendered frames of that
capture, with the same on-screen lines as in the table above.

State it plainly in the narration, for example: *"This is the recorded console
of a QNX 8.0.0 guest running under QEMU — the same build, re-recorded."* Do not
imply a live boot that did not happen.

Other fallbacks, in order of preference:

1. **No SDP at all** → shoot the host segment live (0:20–0:50 and the proofs),
   and render the QNX section from `qnx/capture/runtime-console.txt`.
2. **The live vitrine is down** → drop the optional 8 s VGA-shell shot; the
   interactive shell is not needed for the argument.
3. **A frame renders badly** → fall back to `cat qnx/capture/runtime-console.txt`
   in a terminal and record that window with `x11grab` — the content is the
   same, the framing is simpler.

The conclusion shot (2:40–3:00) never depends on the guest: it is a card plus
the repository, the live demo and the live guest links.

---

## 5. Checklist before publishing

- [ ] Duration is exactly 180 s (`ffprobe` as in §3.4).
- [ ] No music, no third-party logo; only the QNX console output and the
      repository's own screens.
- [ ] The narration says "synthetic sensors", "real QNX 8.0.0 guest under
      QEMU" at least once in the QNX section.
- [ ] Every number shown on screen matches the capture file quoted in §2.
- [ ] No claim of a Raspberry Pi 5, a physical sensor, a shipped security
      policy, or a PPS publisher.
- [ ] Links on the last card resolve: repository, live demo, live guest.
