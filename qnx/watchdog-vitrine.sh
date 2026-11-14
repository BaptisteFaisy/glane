#!/usr/bin/env bash
# Glane — Copyright 2026 Baptiste Faisy. Licence Apache-2.0.
#
# watchdog-vitrine.sh — garde de la vitrine GLANE (invité QNX 8.0 réel sous
# QEMU, exposé en direct sur le web).
#
# Quatre verdicts, pas un seul (leçon du projet Creux, 14/11) :
#   LOCAL   : 127.0.0.1:8101/screen.png répond 200 avec une image non vide
#   MOUVANT : deux cadres espacés de 10 s DIFFÈRENT — le tableau de bord se
#             repeint toutes les 8 s. Un écran FIGÉ passe le test 200/image
#             non vide : c'est exactement le bug d'ems.py du 13/11 (capture
#             VGA figée sur l'écran SeaBIOS alors que l'invité tournait).
#   SÉRIE   : le journal série écrit par glaned DANS l'invité grandit encore.
#             C'est lui qui lève le doute : quand l'invité est mort, web.py
#             sert un PLACEHOLDER animé — le HTTP est 200 et l'image bouge.
#   PUBLIC  : l'URL du funnel répond 200 depuis l'extérieur du poste. Un
#             statut local vert ne prouve pas que le lien du jury est vivant.
#
# Réparation selon le verdict :
#   LOCAL mort OU SÉRIE muette (invité éteint, placeholder animé)
#                   -> relancer les unités (glane-qemu + glane-web)
#   image FIGÉE mais SÉRIE vivante
#                   -> pause légitime du visiteur : on ne touche à rien
#   invité vif (mouvant + série) mais PUBLIC injoignable
#                   -> les services vont bien, c'est le tunnel Funnel qui est
#                      tombé -> reposer le funnel (idempotent)
#   tout vif        -> rien, on écrit juste l'état
#
# Le FIGÉ n'est traité comme une panne que si le journal série est silencieux
# : un visiteur qui met la démo en pause (« echo pause > /dev/shmem/arret »,
# documenté dans qnx/demo-vitrine.sh) fige l'écran SANS planter l'invité —
# glaned continue de publier, le journal grandit, on ne redémarre rien.
#
# Usage : qnx/watchdog-vitrine.sh [--dry-run]
# Tests : WATCH_LOCAL_BASE=… WATCH_PUBLIC_URL=… WATCH_SERIE=… WATCH_STATE_DIR=…
#         pour forcer un chemin ; --dry-run montre la décision sans rien
#         redémarrer.
set -uo pipefail

LOCAL_BASE="${WATCH_LOCAL_BASE:-http://127.0.0.1:8101}"
PUBLIC_URL="${WATCH_PUBLIC_URL:-https://baptiste-ms-7d18.tail3a8bdf.ts.net:10000}"
STATE_DIR="${WATCH_STATE_DIR:-$HOME/Bureau/glane/build-qnx/vitrine}"
SERIE="${WATCH_SERIE:-$STATE_DIR/serial.log}"
FUNNEL_PORT="${FUNNEL_PORT:-10000}"
MIN_PNG=1500          # écran VGA 720x400 compressé : 5-9 ko ; moins = image morte
PAUSE_S=10            # espacement des deux cadres (le cycle se repeint à 8 s)
SILENCE_S=90          # au-delà, le journal série n'a plus été écrit : invité muet
DRY=0
[ "${1:-}" = "--dry-run" ] && DRY=1

log() { printf '%s %s\n' "$(date '+%Y-%m-%d %H:%M:%S')" "$*"; }

# Sonde un chemin : rend 0 si l'image est un PNG 200 non vide, 1 sinon.
sonde_image() {
    local url="$1" out="$2" code size
    code=$(curl -s --max-time 20 -o "$out" -w '%{http_code}' "$url" 2>/dev/null)
    size=$(stat -c %s "$out" 2>/dev/null || echo 0)
    if [ "$code" != "200" ] || [ "$size" -lt "$MIN_PNG" ]; then
        log "SONDE ÉCHEC $url : http=$code size=$size"
        return 1
    fi
    log "SONDE OK   $url : $size octets"
    return 0
}

# L'image bouge-t-elle ? Deux cadres espacés de PAUSE_S comparés par md5.
image_mouvante() {
    local url="$1" a="$2" b="$3"
    sonde_image "$url" "$a" || return 1
    sleep "$PAUSE_S"
    sonde_image "$url" "$b" || return 1
    if [ "$(md5sum < "$a")" = "$(md5sum < "$b")" ]; then
        log "IMAGE FIGÉE : deux cadres identiques à ${PAUSE_S} s ($url)"
        return 1
    fi
    log "IMAGE MOUVANTE : cadres distincts à ${PAUSE_S} s"
    return 0
}

# Le journal série grandit-il ? (preuve que glaned publie encore dans l'invité)
serie_vivante() {
    local age
    [ -f "$SERIE" ] || { log "SÉRIE absent : $SERIE"; return 1; }
    age=$(( $(date +%s) - $(stat -c %Y "$SERIE") ))
    log "SÉRIE : écrit il y a ${age} s ($SERIE)"
    [ "$age" -lt "$SILENCE_S" ]
}

# Repose le tunnel Funnel (idempotent, ne touche pas aux services). Commande
# identique à celle documentée dans qnx/VITRINE.md.
reposer_tunnel() {
    log "ACTION reposer funnel : tailscale funnel --bg --https=$FUNNEL_PORT $LOCAL_BASE"
    [ "$DRY" = "1" ] && return 0
    if ! tailscale funnel --bg --https="$FUNNEL_PORT" "$LOCAL_BASE" >/dev/null 2>&1; then
        log "AVERTISSEMENT : 'tailscale funnel' a échoué (tunnel déjà bon ?)"
    fi
}

# Attend que la vitrine réponde localement (l'invité met ~10 s à amorcer).
attendre_vitrine() {
    local i
    for i in $(seq 1 24); do
        if [ -S "$STATE_DIR/mon.sock" ] \
           && curl -s --max-time 3 -o /dev/null "$LOCAL_BASE/screen.png"; then
            return 0
        fi
        sleep 5
    done
    return 1
}

# Relance les unités de la vitrine (l'invité d'abord, la vitrine web ensuite).
redemarrer_services() {
    log "ACTION redémarrage : glane-qemu puis glane-web"
    [ "$DRY" = "1" ] && return 0
    systemctl --user restart glane-qemu.service
    systemctl --user restart glane-web.service
    if attendre_vitrine; then
        log "vitrine de nouveau vivante localement"
        return 0
    fi
    log "BOUCLE : vitrine toujours morte après 2 min"
    return 1
}

mkdir -p "$STATE_DIR" 2>/dev/null || true
TMP=$(mktemp -d)
trap 'rm -rf "$TMP"' EXIT

# Les quatre verdicts, réévaluables après une réparation.
local_ok=0; mouvant=0; serie_ok=0; public_ok=0
evaluer() {
    local_ok=0; mouvant=0; serie_ok=0; public_ok=0
    sonde_image "$LOCAL_BASE/screen.png" "$TMP/e1.png" && local_ok=1
    [ "$local_ok" = "1" ] \
        && image_mouvante "$LOCAL_BASE/screen.png" "$TMP/e1.png" "$TMP/e2.png" && mouvant=1
    serie_vivante && serie_ok=1
    sonde_image "$PUBLIC_URL/screen.png" "$TMP/e3.png" && public_ok=1
}

evaluer

if [ "$local_ok" = "0" ] || [ "$serie_ok" = "0" ]; then
    # Image absente, OU journal série muet : l'invité ne publie plus. Le cas
    # perfide est le second — web.py sert un PLACEHOLDER animé quand l'invité
    # est mort, donc l'image « bouge » et le HTTP est 200. Seul le journal
    # série (écrit par glaned DANS l'invité) distingue le vrai du faux.
    log "VERDICT : invité mort (local=$local_ok série=$serie_ok) -> réparer les services"
    redemarrer_services
    evaluer
elif [ "$mouvant" = "0" ]; then
    if serie_ok; then
        log "VERDICT : écran figé MAIS journal série vivant -> pause du visiteur, on ne touche pas"
    else
        log "VERDICT : écran figé et série muette -> réparer les services"
        redemarrer_services
        evaluer
    fi
elif [ "$public_ok" = "0" ]; then
    log "VERDICT : invité vif, PUBLIC injoignable -> tunnel Funnel tombé (pas les services)"
    reposer_tunnel
    sleep 5
    sonde_image "$PUBLIC_URL/screen.png" "$TMP/p2.png" && public_ok=1
else
    log "VERDICT : vitrine live (image mouvante + série vivante + public)"
fi

# Cumul écrit dans un fichier dédié (leçon Creux : un contrôle n'écrase pas
# l'autre). « live » exige l'image mouvante, la série vivante et le public.
cumul="mort"
[ "$local_ok" = "1" ] && [ "$serie_ok" = "1" ] && cumul="invité-vif-sans-public"
[ "$mouvant" = "1" ] && [ "$serie_ok" = "1" ] && [ "$public_ok" = "1" ] && cumul="live"
printf '{"ts":"%s","local":%s,"mouvant":%s,"serie":%s,"public":%s,"etat":"%s","public_url":"%s","dry_run":%s}\n' \
    "$(date '+%Y-%m-%dT%H:%M:%S%z')" "$local_ok" "$mouvant" "$serie_ok" "$public_ok" \
    "$cumul" "$PUBLIC_URL" "$DRY" > "$STATE_DIR/watchdog.json"
log "état : $cumul -> $STATE_DIR/watchdog.json"
[ "$cumul" = "live" ] && exit 0
exit 1
