#!/bin/sh
# Lance le runner GitHub Actions « pc-qnx-repo » (dépôt Tradoooo/glane,
# job QNX 8.0). Le PATH doit contenir /usr/bin : le job compile pour QNX
# puis amorce l'IFS dans QEMU — sans qemu-system-x86_64 dans le PATH, le
# job échoue sur une erreur trompeuse.
export PATH="/home/baptiste/.local/bin:/home/baptiste/bin:/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin"
exec ./bin/Runner.Listener run
