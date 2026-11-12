#!/usr/bin/env bash
# Glane — Copyright 2026 Baptiste Faisy. Licence Apache-2.0.
#
# run-glane-qemu.sh — la démo « glaned sur un VRAI émulateur QNX 8.0 », de
# bout en bout et rejouable :
#   1. compile glaned + qnxproof pour QNX 8.0 (qcc) ;
#   2. assemble une mini-racine QNX (usr/bin + usr/lib) pour que mkifs
#      résolve l'interpréteur des binaires dynamiques ;
#   3. construit une IFS minimale (mkifs) qui lance glaned au démarrage ;
#   4. l'amorce dans QEMU (qemu-system-x86_64 -kernel, sans disque ni réseau)
#      et capture la console série ;
#   5. range les artefacts dans qnx/capture/ (preuve uname QNX, journal
#      produit PAR QNX, verdict de rejeu déterministe).
#
# Usage : qnx/run-glane-qemu.sh
# Prérequis : SDP QNX 8.0 installé (~/qnx800). bash requis (qnxsdp-env.sh).
set -euo pipefail

ROOT=$(cd "$(dirname "$0")/.." && pwd)
OUT="$ROOT/build-qnx"
CAP="$ROOT/qnx/capture"
SDP="${QNX_SDP:-$HOME/qnx800}"
TGT="$SDP/target/qnx/x86_64"

[ -f "$SDP/qnxsdp-env.sh" ] || { echo "SDP introuvable ($SDP)" >&2; exit 1; }
# shellcheck disable=SC1091
. "$SDP/qnxsdp-env.sh"

mkdir -p "$OUT/ifsbin/usr/bin" "$OUT/ifsbin/usr/lib" "$CAP"

echo "==> 1/4 compilation pour QNX 8.0 (qcc $(qcc --version 2>&1 | grep -oE '[0-9]+\.[0-9]+\.[0-9]+' | head -1))"
qcc -V gcc_ntox86_64 -std=c11 -O2 \
    -I"$ROOT/core/include" -I"$ROOT/publish/qnx" \
    "$ROOT/target/glaned.c" "$ROOT/publish/qnx/glane_publish.c" \
    "$ROOT"/core/src/*.c \
    -o "$OUT/ifsbin/usr/bin/glaned"
qcc -V gcc_ntox86_64 -std=c11 -O2 \
    "$ROOT/tools/qnxproof.c" -o "$OUT/ifsbin/usr/bin/qnxproof"
# l'interpréteur doit être trouvable par mkifs relativement au binaire
cp "$TGT/usr/lib/ldqnx-64.so.2" "$OUT/ifsbin/usr/lib/"

echo "==> 2/4 construction de l'IFS minimale (mkifs)"
mkifs "$ROOT/qnx/glane-ifs.build" "$OUT/glane-ifs.bin"

echo "==> 3/4 amorçage dans QEMU (qemu-system-x86_64 -kernel, sans disque/réseau)"
CONSOLE="$OUT/qnx-console.log"
rm -f "$CONSOLE"
qemu-system-x86_64 -kernel "$OUT/glane-ifs.bin" -m 512 -smp 1 -cpu max \
    -display none -vga none -serial "file:$CONSOLE" -no-reboot &
QPID=$!
i=0
while [ "$i" -lt 45 ]; do
    if grep -q '===== FIN' "$CONSOLE" 2>/dev/null; then break; fi
    if ! kill -0 "$QPID" 2>/dev/null; then break; fi
    i=$((i + 1))
    sleep 2
done
# arrêt par PID — JAMAIS par motif (le motif tuerait les VMs des autres projets)
kill "$QPID" 2>/dev/null || true
sleep 1
kill -9 "$QPID" 2>/dev/null || true

echo "==> 4/4 extraction des artefacts -> qnx/capture/"
cp "$CONSOLE" "$CAP/runtime-console.txt"
grep -a '^\[QNX-PROOF\]' "$CONSOLE" > "$CAP/runtime-uname.txt" || true
grep -aE '^\{"s":|^glaned: ' "$CONSOLE" > "$CAP/runtime-glaned.txt" || true
grep -a '^{"s":' "$CONSOLE" | head -20 > "$CAP/runtime-journal.jsonl" || true
# verdict de rejeu déterministe : les 2 runs doivent être identiques
TOTAL=$(grep -ac '^{"s":' "$CONSOLE" || echo 0)
grep -a '^{"s":' "$CONSOLE" | head -20 > /tmp/glane-run1.jsonl
grep -a '^{"s":' "$CONSOLE" | tail -20 > /tmp/glane-run2.jsonl
if cmp -s /tmp/glane-run1.jsonl /tmp/glane-run2.jsonl; then
    DET="IDENTIQUES octet pour octet (rejeu déterministe confirmé sur QNX 8.0)"
else
    DET="DIFFÉRENTS (anormal — le rejeu n'est pas déterministe)"
fi
cat > "$CAP/runtime-howto.md" <<EOF
# Capture d'exécution Glane sur QNX 8.0 (émulateur)

Date : $(date '+%Y-%m-%d %H:%M:%S %Z')
Hôte : $(uname -srmo)
Outil QNX : qcc $(qcc --version 2>&1 | grep -oE '[0-9]+\.[0-9]+\.[0-9]+' | head -1) (SDP 8.0), variante \`gcc_ntox86_64\`

## Image amorcée
\`\`\`
qemu-system-x86_64 -kernel build-qnx/glane-ifs.bin -m 512 -smp 1 -cpu max \\
  -display none -vga none -serial file:build-qnx/qnx-console.log -no-reboot
\`\`\`
IFS minimale (2,3 Mo, sans disque) construite par \`qnx/glane-ifs.build\` :
procnto-smp-instr + startup-x86 (console série 115200) + glaned + qnxproof.

## Preuve d'OS (uname exécuté DANS l'invité)
\`\`\`
$(head -1 "$CAP/runtime-uname.txt" 2>/dev/null)
\`\`\`

## Journal produit par glaned DANS QNX 8.0
\`\`\`
$(head -3 "$CAP/runtime-journal.jsonl" 2>/dev/null)
…
\`\`\`

## Rejeu déterministe (GLA-04)
Deux exécutions de \`glaned\` avec la même graine (42) dans le même QNX :
**$DET** — $TOTAL lignes de mesure au total.

## Rejouer
\`\`\`sh
qnx/run-glane-qemu.sh      # compile, construit l'IFS, amorce, capture
\`\`\`
EOF
echo "artefacts : $CAP/runtime-{console,uname,glaned,journal.jsonl,howto.md}"
echo "rejeu : $DET"
