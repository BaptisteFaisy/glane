#!/usr/bin/env python3
"""
Glane — Copyright 2026 Baptiste Faisy. Licence Apache-2.0.

glane_record.py — capture LIVE de l'invité QNX 8.0 pour la vidéo de démo.

On enregistre trois choses, sans jamais rien simuler :

  1. la CROISSANCE du journal de la console série (le fichier grandit en
     direct pendant que l'invité démarre et publie) : on note (t, octets lus)
     à chaque apparition de données — c'est la chronologie réelle ;
  2. des captures de l'ÉCRAN VGA de l'invité (via la vitrine HTTP, endpoint
     /screen.png) à intervalle régulier ;
  3. l'injection de touches (endpoint /key) pour montrer que l'invité
     répond au clavier — l'ordre et l'instant de chaque touche sont notés.

Sortie : <out>/console.timeline.json, <out>/vga/*.png, <out>/meta.json.

Usage :
    python3 scripts/glane_record.py --out build/video/src \
        --console build-qnx/vitrine/serial.log --duration 60 \
        --base http://127.0.0.1:8101 --keys "40:ls /dev/shmem;42:cat /dev/shmem/temp-chassis.info"
"""
import argparse
import json
import os
import time
import urllib.parse
import urllib.request

VGA_EVERY = 1.0          # une capture VGA par seconde
KEY_CHAR_DELAY = 0.06     # délai entre deux touches injectées

# Noms de touches du moniteur QEMU pour les caractères de la commande.
QNX_KEYS = {" ": "spc", "/": "slash", "-": "minus", ".": "dot", "_": "shift-minus"}


def get(url, timeout=5):
    """GET HTTP ; rend les octets ou None en cas d'échec."""
    try:
        with urllib.request.urlopen(url, timeout=timeout) as r:
            return r.read()
    except Exception:
        return None


def parse_keys(spec):
    """'40:ls /dev/shmem;42:cat x' -> [(40.0, ['l','s',...]), ...].

    Chaque segment est « seconde:commande » ; la commande est injectée
    caractère par caractère, puis par « ret » (RETURN). Les caractères sont
    traduits en noms de touches QEMU (moniteur) : espace -> spc, / -> slash.
    """
    jobs = []
    for part in spec.split(";"):
        part = part.strip()
        if not part:
            continue
        when, _, cmd = part.partition(":")
        keys = [QNX_KEYS.get(c, c) for c in cmd] + ["ret"]
        jobs.append((float(when), keys))
    jobs.sort()
    return jobs


def read_new(path, offset):
    """Lit les octets disponibles depuis `offset` ; rend (données, offset).

    Si le journal a été tronqué (redémarrage de l'invité), on repart de 0.
    """
    try:
        size = os.path.getsize(path)
    except OSError:
        return b"", offset
    if size < offset:
        offset = 0
    if size <= offset:
        return b"", offset
    with open(path, "rb") as f:
        f.seek(offset)
        data = f.read()
    return data, offset + len(data)


def grab_vga(base, out_dir, index):
    """Capture l'écran VGA ; rend l'index écrit ou None."""
    data = get(base + "/screen.png")
    if not data or not data.startswith(b"\x89PNG"):
        return None
    path = os.path.join(out_dir, "vga", "%04d.png" % index)
    with open(path, "wb") as f:
        f.write(data)
    return index


def inject(base, keys):
    """Injecte une suite de touches ; rend la liste des touches envoyées."""
    sent = []
    for k in keys:
        url = base + "/key?k=" + urllib.parse.quote(k)
        if get(url) is not None:
            sent.append(k)
        time.sleep(KEY_CHAR_DELAY)
    return sent


def record(args):
    """Boucle de capture : chronologie console + VGA + touches."""
    out = args.out
    os.makedirs(os.path.join(out, "vga"), exist_ok=True)
    t0 = time.time()
    offset = 0
    timeline = []
    vga = []
    keys_log = []
    next_vga = 0.0
    pending = list(args.keys)

    while True:
        now = time.time() - t0
        if now > args.duration:
            break
        data, offset = read_new(args.console, offset)
        if data:
            timeline.append({"t": round(now, 3), "bytes": len(data)})
        if now >= next_vga:
            idx = grab_vga(args.base, out, len(vga))
            if idx is not None:
                vga.append({"t": round(now, 3), "file": "vga/%04d.png" % idx})
            next_vga = now + args.vga_every
        while pending and pending[0][0] <= now:
            when, keys = pending.pop(0)
            sent = inject(args.base, keys)
            keys_log.append({"t": round(now, 3), "keys": "".join(keys),
                             "sent": len(sent)})
        time.sleep(0.05)

    meta = {
        "duree": round(time.time() - t0, 2),
        "console": args.console,
        "timeline": timeline,
        "vga": vga,
        "touches": keys_log,
    }
    with open(os.path.join(out, "console.timeline.json"), "w") as f:
        json.dump(meta, f, indent=1)
    # copie du journal complet : le rendu le relit comme une seule image texte
    cdir = os.path.join(out, "console")
    os.makedirs(cdir, exist_ok=True)
    try:
        with open(args.console, "rb") as src_f, \
                open(os.path.join(cdir, "serial.log"), "wb") as dst_f:
            dst_f.write(src_f.read())
    except OSError:
        pass
    print("console: %d evenements, %d captures VGA, %d injections"
          % (len(timeline), len(vga), len(keys_log)))
    return meta


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--out", required=True, help="dossier de sortie")
    ap.add_argument("--console", required=True, help="journal serie de l'invite")
    ap.add_argument("--base", default="http://127.0.0.1:8101", help="vitrine HTTP")
    ap.add_argument("--duration", type=float, default=60.0)
    ap.add_argument("--vga-every", type=float, default=VGA_EVERY)
    ap.add_argument("--keys", default="", help="'40:ls /dev/shmem;45:cat x'")
    args = ap.parse_args()
    args.keys = parse_keys(args.keys)
    record(args)


if __name__ == "__main__":
    main()
