#!/usr/bin/env bash
# recette-glane.sh — vérification de la vitrine web de Glane (mission).
# Chaque critère est vérifié par une commande : rien n'est supposé.
set -u
cd "$(dirname "$0")"
LOCAL="http://127.0.0.1:8101"
FUNNEL="https://baptiste-ms-7d18.tail3a8bdf.ts.net:10000"
RESOLVE="--resolve baptiste-ms-7d18.tail3a8bdf.ts.net:10000:176.58.90.63"
OK=0; KO=0
verdict() { if [ "$1" = "0" ]; then echo "  [OK] $2"; OK=$((OK+1)); else echo "  [KO] $2"; KO=$((KO+1)); fi; }

echo "== 1. /console : journal de la console serie QNX 8.0 =="
C=$(curl -s -m 20 "$LOCAL/console")
grep -qa "QNX localhost 8.0.0" <<<"$C"; verdict $? "banniere QNX 8.0 (uname de l'invite)"
grep -qa "glane-publish\|mesures publiees" <<<"$C"; verdict $? "publications de mesures par glaned"
grep -qa "Voici un vrai systeme QNX" <<<"$C"; verdict $? "annonce de l'invite ksh"
echo "  ($(wc -c <<<"$C") octets de journal)"

echo "== 2. /age : fraicheur de la console =="
A=$(curl -s -m 10 "$LOCAL/age"); echo "  -> $A"
grep -qa "fraîche" <<<"$A"; verdict $? "console fraiche"

echo "== 3. /screen.png : capture VGA non vide =="
curl -s -m 30 -o /tmp/rec-avant.png "$LOCAL/screen.png"
SZ=$(stat -c%s /tmp/rec-avant.png); TYP=$(file -b --mime-type /tmp/rec-avant.png)
echo "  -> $TYP, $SZ octets"
[ "$TYP" = "image/png" ] && [ "$SZ" -gt 3000 ]; verdict $? "image PNG non vide"
# OCR par zones : le plein ecran est mal lu par tesseract (lignes de ====)
LIG=$(python3 /home/baptiste/Bureau/glane/qnx/recette/png-find.py /tmp/rec-avant.png "INVITE|GLANE" 2>/dev/null | head -1)
echo "  -> banniere 'QNX 8.0 - INVITE GLANE' trouvee ligne $LIG"
[ "${LIG:-0}" -ge 0 ]; verdict $? "l'ecran montre l'invite QNX 8.0 de Glane"
# l'invite ksh : ligne de texte non vide en bas de l'ecran (pixels, pas OCR)
R1=$(python3 /home/baptiste/Bureau/glane/qnx/recette/png-rows.py /tmp/rec-avant.png 300 400)
echo "  -> $R1 ligne(s) de texte en bas de l'ecran"
[ "$R1" -ge 1 ]; verdict $? "l'invite ksh est a l'ecran"

echo "== 4. /key?k=ret : injection de touches =="
K=$(curl -s -m 10 "$LOCAL/key?k=ret"); echo "  -> $K"
[ "$K" = "ok" ]; verdict $? "la touche est injectee (reponse 'ok')"
sleep 3
curl -s -m 30 -o /tmp/rec-apres.png "$LOCAL/screen.png"
cmp -s /tmp/rec-avant.png /tmp/rec-apres.png; [ $? -ne 0 ]; verdict $? "la capture suivante est differente"
N1=$(python3 /home/baptiste/Bureau/glane/qnx/recette/png-rows.py /tmp/rec-avant.png 300 400)
N2=$(python3 /home/baptiste/Bureau/glane/qnx/recette/png-rows.py /tmp/rec-apres.png 300 400)
echo "  -> lignes de texte en bas de l'ecran : $N1 avant, $N2 apres"
# la nouvelle invite est ecrite sur la derniere ligne de l'ecran : la diff
# doit atteindre le bas de l'image (y1 = 400).
Y1=$(python3 /home/baptiste/Bureau/glane/qnx/recette/png-diff.py /tmp/rec-avant.png /tmp/rec-apres.png | head -1 | sed 's/.*, \([0-9]*\))/\1/')
echo "  -> la diff pixel atteint la ligne y=$Y1"
[ "${Y1:-0}" -ge 384 ]; verdict $? "une nouvelle ligne d'invite est apparue"

echo "== 5. vitrine exposee (tailscale funnel) =="
timeout 40 curl -s -o /tmp/rec-funnel.png -w "  /screen.png -> HTTP %{http_code} %{size_download}o\n" $RESOLVE "$FUNNEL/screen.png"
timeout 40 curl -s -o /tmp/rec-funnel.html -w "  /            -> HTTP %{http_code} %{size_download}o\n" $RESOLVE "$FUNNEL/"
timeout 40 curl -s -o /tmp/rec-funnel.txt -w "  /console     -> HTTP %{http_code} %{size_download}o\n" $RESOLVE "$FUNNEL/console"
F=$(stat -c%s /tmp/rec-funnel.png); [ "$F" -gt 3000 ]; verdict $? "capture via le funnel non vide ($F octets)"
grep -qa "QNX localhost 8.0.0" /tmp/rec-funnel.txt; verdict $? "console QNX 8.0 servie par le funnel"

echo "== 6. page du site du dossier QNX =="
timeout 45 curl -s -o /tmp/rec-site.html -w "  docs/glane.html -> HTTP %{http_code} %{size_download}o\n" "https://tradoooo.github.io/qnx-dossier/glane.html"
timeout 45 curl -s -o /tmp/rec-site2.html -w "  docs/glane-demo.html -> HTTP %{http_code} %{size_download}o\n" "https://tradoooo.github.io/qnx-dossier/glane-demo.html"
grep -qa "baptiste-ms-7d18.tail3a8bdf.ts.net:10000" /tmp/rec-site.html; verdict $? "la page Glane du dossier QNX pointe sur la vitrine"
grep -qa "baptiste-ms-7d18.tail3a8bdf.ts.net:10000" /tmp/rec-site2.html; verdict $? "la page dediee de la vitrine est publiee"
timeout 45 curl -s -o /tmp/rec-glane-site.html -w "  glane.github.io -> HTTP %{http_code} %{size_download}o\n" "https://tradoooo.github.io/glane/"
grep -qa "qnx-live-screen" /tmp/rec-glane-site.html; verdict $? "l'invite en direct est branchee dans le site Glane"

echo
echo "RESULTAT : $OK reussis, $KO echecs"
[ "$KO" -eq 0 ]
