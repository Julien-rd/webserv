#!/usr/bin/env python3
import os, sys

VARS = ["REQUEST_METHOD", "QUERY_STRING", "PATH_INFO", "SCRIPT_NAME",
        "SCRIPT_FILENAME", "CONTENT_TYPE", "CONTENT_LENGTH",
        "SERVER_PROTOCOL", "GATEWAY_INTERFACE"]

body = ""
n = os.environ.get("CONTENT_LENGTH", "")
if n.isdigit() and int(n) > 0:
    body = sys.stdin.read(int(n))

print("Content-Type: text/plain\r")
print("\r")
for v in VARS:
    print("%-18s %r" % (v, os.environ.get(v)))
print("%-18s %d bytes" % ("BODY", len(body)))