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
qcc -V gcc_ntox86_64 -std=c11 -O2 \
    "$ROOT/target/gwatch.c" -o "$OUT/ifsbin/usr/bin/gwatch"
qcc -V gcc_ntox86_64 -std=c11 -O2 \
    "$ROOT/target/gsensor_resmgr.c" -o "$OUT/ifsbin/usr/bin/gsensor_resmgr"
qcc -V gcc_ntox86_64 -std=c11 -O2 \
    "$ROOT/target/gsuper.c" -o "$OUT/ifsbin/usr/bin/gsuper"
# l'interpréteur doit être trouvable par mkifs relativement au binaire
cp "$TGT/usr/lib/ldqnx-64.so.2" "$OUT/ifsbin/usr/lib/"
# Outils : toybox dispatche sur argv[0] -> une copie par nom d'appel.
# (mkifs refuse le mélange 'nom = chemin' et [type=link].)
# printf + clear : indispensables au tableau de bord de la vitrine (devc-con
# comprend les suites ANSI, mais toybox doit etre present sous ce nom).
for t in echo uname ls cat tail mkdir sleep printf clear head wc; do
    [ -f "$TGT/usr/bin/toybox" ] && cp "$TGT/usr/bin/toybox" "$OUT/ifsbin/usr/bin/$t"
done
for t in ksh sh; do
    [ -f "$TGT/bin/ksh" ] && cp "$TGT/bin/ksh" "$OUT/ifsbin/usr/bin/$t"
done
[ -f "$TGT/bin/pidin" ] && cp "$TGT/bin/pidin" "$OUT/ifsbin/usr/bin/pidin"

echo "==> 2/4 construction de l'IFS minimale (mkifs)"
mkifs "$ROOT/qnx/glane-ifs.build" "$OUT/glane-ifs.bin"

echo "==> 3/4 amorçage dans QEMU (qemu-system-x86_64 -kernel, sans disque/réseau)"
# Le binaire QEMU est cherché par son NOM COMPLET : sur un runner où le PATH
# du service est minimal, « qemu-system-x86_64 » seul n'est pas trouvé et le
# job échoue sur une erreur trompeuse (« commande introuvable ») alors que
# QEMU est installé. On dit explicitement ce qui manque.
QEMU_BIN="${QEMU_BIN:-$(command -v qemu-system-x86_64 || true)}"
if [ -z "$QEMU_BIN" ] || [ ! -x "$QEMU_BIN" ]; then
    for candidat in /usr/bin/qemu-system-x86_64 /usr/local/bin/qemu-system-x86_64; do
        [ -x "$candidat" ] && QEMU_BIN="$candidat" && break
    done
fi
if [ -z "$QEMU_BIN" ]; then
    echo "ERREUR: qemu-system-x86_64 introuvable (PATH=$PATH)." >&2
    echo "Ce job exige QEMU : le runner doit avoir QEMU installé (labels 'qnx')." >&2
    exit 1
fi
echo "    qemu : $QEMU_BIN"
CONSOLE="$OUT/qnx-console.log"
rm -f "$CONSOLE"
"$QEMU_BIN" -kernel "$OUT/glane-ifs.bin" -m 512 -smp 1 -cpu max \
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
# Section "publication réelle" : message du publisher + ls des objets + valeurs
awk '/glane-publish:/{f=1} f' "$CONSOLE" | awk '/^=====/{exit} {print}' > "$CAP/runtime-glaned.txt" || true
grep -a '^{"s":' "$CONSOLE" | head -20 > "$CAP/runtime-journal.jsonl" || true
# verdict de rejeu déterministe : les 2 runs REJOUÉS DANS L'INVITÉ sont
# comparés octet pour octet (les blocs REJEU-RUN1 / REJEU-RUN2 du guest).
rm -f /tmp/glane-rejeu1.txt /tmp/glane-rejeu2.txt
awk '/^REJEU-RUN1 ===/{f=1;next} /^REJEU-RUN1-FIN ===/{f=0;next}
     /^REJEU-RUN2 ===/{f=2;next} /^REJEU-RUN2-FIN ===/{f=0;next}
     f==1{sub(/\r$/, ""); print > "/tmp/glane-rejeu1.txt"}
     f==2{sub(/\r$/, ""); print > "/tmp/glane-rejeu2.txt"}' "$CONSOLE"
TOTAL=$(wc -l < /tmp/glane-rejeu1.txt 2>/dev/null || echo 0)
if [ -s /tmp/glane-rejeu1.txt ] && cmp -s /tmp/glane-rejeu1.txt /tmp/glane-rejeu2.txt; then
    DET="IDENTIQUES octet pour octet ($TOTAL mesures comparées, rejouées DANS QNX 8.0)"
else
    DET="DIFFÉRENTS ou vides (anormal — le rejeu n'est pas déterministe)"
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
IFS minimale ($(du -h "$OUT/glane-ifs.bin" | cut -f1), sans disque) construite par \`qnx/glane-ifs.build\` :
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
Deux exécutions de \`glaned\` avec la MÊME description, rejouées DANS l'invité
QNX 8.0 (graine dérivée de l'index du capteur, donc identique à chaque run) :
**$DET**

## Rejouer
\`\`\`sh
qnx/run-glane-qemu.sh      # compile, construit l'IFS, amorce, capture
\`\`\`
EOF
echo "artefacts : $CAP/runtime-{console,uname,glaned,journal.jsonl,howto.md}"
echo "rejeu : $DET"
