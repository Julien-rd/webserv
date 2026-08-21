#!/bin/bash
# Usage: ./methods.sh [http://host:port] [pfad]
BASE=${1:-http://127.0.0.1:7070}
P=${2:-/login/}
c() { curl -s -o /dev/null -m 5 -w '%{http_code}' "$@" 2>/dev/null || echo "---"; }
row() { printf "%-10s %-6s %s\n" "$1" "$2" "$3"; }

echo "== implementiert =="
row GET     "$(c              $BASE$P)"  "200"
row POST    "$(c -X POST -d 'a=1' $BASE$P)" "200 / 405 / 400"
row DELETE  "$(c -X DELETE    $BASE$P)"  "204 / 403 / 405"

echo; echo "== bekannt, aber nicht erlaubt -> 405 =="
row HEAD    "$(c -I           $BASE$P)"  "405  (200 falls implementiert)"
row PUT     "$(c -X PUT -d x  $BASE$P)"  "405"
row OPTIONS "$(c -X OPTIONS   $BASE$P)"  "405"
row TRACE   "$(c -X TRACE     $BASE$P)"  "405"
row CONNECT "$(c -X CONNECT   $BASE$P)"  "405"
row PATCH   "$(c -X PATCH -d x $BASE$P)" "405"

echo; echo "== unbekannt -> 501 =="
row BREW    "$(c -X BREW      $BASE$P)"  "501"
row get     "$(c -X get       $BASE$P)"  "501 (case-sensitive!)"
row LOOOONG "$(c -X VERYLONGMETHODNAME $BASE$P)" "501"

echo; echo "== kein gueltiges Token -> 400 (optional) =="
row 'GE{T'  "$(c -X 'GE{T'    $BASE$P)"  "400 oder 501"

echo; echo "== Allow-Header bei 405 (Pflicht) =="
curl -s -D - -o /dev/null -m 5 -X PUT -d x $BASE$P | grep -iE '^HTTP/|^Allow:' | tr -d '\r'