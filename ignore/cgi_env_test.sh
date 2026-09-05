#!/bin/bash
BASE=${1:-http://127.0.0.1:7070}
S=${2:-/cgitest/env.py}
run() { echo "--- $1"; echo "    erwartet: $2"; curl -s -m 5 --path-as-is "$BASE$3" | sed 's/^/    /'; echo; }

run "einfacher Query"       "QUERY_STRING 'x=1'"                 "$S?x=1"
run "mehrere Parameter"     "QUERY_STRING 'a=1&b=2'"             "$S?a=1&b=2"
run "kodiertes & im Wert"   "QUERY_STRING 'name=a%26b=evil'"     "$S?name=a%26b=evil"
run "kodiertes Leerzeichen" "QUERY_STRING 'q=hello%20world'"     "$S?q=hello%20world"
run "leerer Query"          "QUERY_STRING '' , Code 200"         "$S?"
run "kein Query"            "QUERY_STRING ''"                    "$S"
run "PATH_INFO"             "PATH_INFO '/extra/pfad'"            "$S/extra/pfad?x=1"

echo "--- POST mit Body"
echo "    erwartet: REQUEST_METHOD 'POST', CONTENT_LENGTH '10', BODY 10 bytes"
curl -s -m 5 -d "hallo=welt" "$BASE$S?z=9" | sed 's/^/    /'