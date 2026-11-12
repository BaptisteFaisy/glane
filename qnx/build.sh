#!/bin/sh
# Glane — Copyright 2026 Baptiste Faisy. Licence Apache-2.0.
#
# qnx/build.sh — compile glaned + le socle core/ pour QNX 8.0 (x86_64) avec qcc.
#
# Prérequis : QNX SDP 8.0 installé (défaut ~/qnx800) et licence déployée.
# Usage : qnx/build.sh [chemin-de-sortie]
#
# Le socle core/ est du C11 portable pur : il compile tel quel avec qcc.
# Seuls target/glaned.c et publish/qnx/ touchent à l'OS.
set -eu

SDP="${QNX_SDP:-$HOME/qnx800}"
if [ ! -f "$SDP/qnxsdp-env.sh" ]; then
    echo "ERREUR: SDP introuvable ($SDP). Installe QNX SDP 8.0 via QNX Software Center." >&2
    exit 1
fi
# shellcheck disable=SC1091
. "$SDP/qnxsdp-env.sh"

ROOT=$(cd "$(dirname "$0")/.." && pwd)
OUT="${1:-$ROOT/build-qnx/glaned}"
# Variante de toolchain QNX 8.0 x86_64 (à ajuster selon `qcc -V` listé par le SDP).
VARIANT="${QNX_VARIANT:-gcc_ntox86_64le}"

mkdir -p "$(dirname "$OUT")"
echo "Compilation QNX (variante $VARIANT) -> $OUT"
# -Werror activé une fois le build validé sur le SDP ; retiré pour le premier port.
qcc -V "$VARIANT" -std=c11 -O2 -Wall -Wextra \
    -I"$ROOT/core/include" -I"$ROOT/publish/qnx" \
    "$ROOT/target/glaned.c" \
    "$ROOT/publish/qnx/glane_publish.c" \
    "$ROOT"/core/src/*.c \
    -o "$OUT"

echo "OK: $OUT"
file "$OUT" 2>/dev/null || true