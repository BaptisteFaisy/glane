# Capture d'exécution Glane sur QNX 8.0 (émulateur)

Date : 2026-11-13 14:12:47 CET
Hôte : Linux 7.0.0-43-generic x86_64 GNU/Linux
Outil QNX : qcc 12.2.0 (SDP 8.0), variante `gcc_ntox86_64`

## Image amorcée
```
qemu-system-x86_64 -kernel build-qnx/glane-ifs.bin -m 512 -smp 1 -cpu max \
  -display none -vga none -serial file:build-qnx/qnx-console.log -no-reboot
```
IFS minimale (5,7M, sans disque) construite par `qnx/glane-ifs.build` :
procnto-smp-instr + startup-x86 (console série 115200) + glaned + qnxproof.

## Preuve d'OS (uname exécuté DANS l'invité)
```
[QNX-PROOF] sysname=QNX release=8.0.0 version=2026/06/05-16:26:03EDT machine=x86pc node=localhost
```

## Journal produit par glaned DANS QNX 8.0
```
{"s":"imu-accel","id":2,"seq":56,"ts_ns":550000000,"v_m":5750,"unit":"g","quality":"ok"}
{"s":"imu-accel","id":2,"seq":57,"ts_ns":560000000,"v_m":6000,"unit":"g","quality":"ok"}
{"s":"imu-accel","id":2,"seq":58,"ts_ns":570000000,"v_m":6250,"unit":"g","quality":"ok"}
…
```

## Rejeu déterministe (GLA-04)
Deux exécutions de `glaned` avec la MÊME description, rejouées DANS l'invité
QNX 8.0 (graine dérivée de l'index du capteur, donc identique à chaque run) :
**IDENTIQUES octet pour octet (6 mesures comparées, rejouées DANS QNX 8.0)**

## Rejouer
```sh
qnx/run-glane-qemu.sh      # compile, construit l'IFS, amorce, capture
```
