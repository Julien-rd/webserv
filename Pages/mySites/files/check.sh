#!/bin/bash
# Usage: ./check.sh [host:port] [pfad-prefix]
HOST="${1:-127.0.0.1:7070}"
PREFIX="${2:-}"
DIR="$(cd "$(dirname "$0")" && pwd)"
FAIL=0

echo "=== 1) Groesse + Inhalt identisch? ==="
for f in tiny medium large boundary; do
    exp=$(stat -c%s "$DIR/$f.html")
    got=$(curl -s "http://$HOST$PREFIX/$f.html" | wc -c)
    md5a=$(md5sum < "$DIR/$f.html" | cut -d' ' -f1)
    md5b=$(curl -s "http://$HOST$PREFIX/$f.html" | md5sum | cut -d' ' -f1)
    if [ "$exp" = "$got" ] && [ "$md5a" = "$md5b" ]; then
        echo "  OK    $f.html  ($exp bytes)"
    else
        echo "  FAIL  $f.html  erwartet $exp, bekommen $got  (md5 $md5a vs $md5b)"
        FAIL=1
    fi
done

echo "=== 2) Content-Length == echte Bodylaenge? ==="
for f in tiny medium large boundary; do
    cl=$(curl -s -D- -o /dev/null "http://$HOST$PREFIX/$f.html" | grep -i '^content-length' | tr -d '\r' | awk '{print $2}')
    real=$(curl -s "http://$HOST$PREFIX/$f.html" | wc -c)
    [ "$cl" = "$real" ] && echo "  OK    $f.html  ($cl)" || { echo "  FAIL  $f.html  header=$cl real=$real"; FAIL=1; }
done

echo "=== 3) Keep-Alive: 3 Requests auf einer Verbindung ==="
n=$(curl -s -o /dev/null -w '%{http_code}\n' \
    "http://$HOST$PREFIX/tiny.html" \
    "http://$HOST$PREFIX/medium.html" \
    "http://$HOST$PREFIX/tiny.html" | grep -c 200)
[ "$n" = "3" ] && echo "  OK    3/3" || { echo "  FAIL  nur $n/3"; FAIL=1; }

echo "=== 4) Langsamer Leser (erzwingt partielle sends) ==="
got=$(curl -s --limit-rate 200k "http://$HOST$PREFIX/large.html" | wc -c)
exp=$(stat -c%s "$DIR/large.html")
[ "$exp" = "$got" ] && echo "  OK    $got bytes" || { echo "  FAIL  erwartet $exp, bekommen $got"; FAIL=1; }

echo "=== 5) 10 parallele Downloads der grossen Datei ==="
ok=0
for i in $(seq 1 10); do
    ( [ "$(curl -s "http://$HOST$PREFIX/large.html" | wc -c)" = "$(stat -c%s "$DIR/large.html")" ] && echo x ) &
done > /tmp/par.$$ 2>/dev/null
wait
ok=$(wc -c < /tmp/par.$$); rm -f /tmp/par.$$
[ "$ok" = "10" ] && echo "  OK    10/10" || { echo "  FAIL  nur $ok/10"; FAIL=1; }

echo "=== 6) Client bricht mittendrin ab (Server darf nicht sterben) ==="
curl -s --max-time 0.05 "http://$HOST$PREFIX/large.html" > /dev/null 2>&1
sleep 0.3
curl -s -o /dev/null -w '  danach erreichbar: %{http_code}\n' "http://$HOST$PREFIX/tiny.html"

echo "=== 7) Pipelining (2 Requests in einem TCP-Paket) ==="
p=$(printf 'GET %s/tiny.html HTTP/1.1\r\nHost: x\r\n\r\nGET %s/tiny.html HTTP/1.1\r\nHost: x\r\nConnection: close\r\n\r\n' "$PREFIX" "$PREFIX" \
    | timeout 5 nc ${HOST%:*} ${HOST#*:} | grep -c '^HTTP/1.1')
[ "$p" = "2" ] && echo "  OK    2 Antworten" || { echo "  HINWEIS  $p Antworten (2 erwartet)"; }

echo
[ "$FAIL" = "0" ] && echo "==> statischer Pfad sauber" || echo "==> Fehler im statischen Pfad"
exit $FAIL
