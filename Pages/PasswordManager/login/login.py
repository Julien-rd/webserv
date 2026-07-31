#!/usr/bin/env python3
import sys
import os
import json
import urllib.parse
import secrets
import time

EXPIRESESSION = 60 # in seconds

# webserv sends POST body via stdin
body = sys.stdin.read(int(os.environ.get('CONTENT_LENGTH', 0)))
fields = urllib.parse.parse_qs(body)

email    = fields.get('email', [''])[0]
password = fields.get('password', [''])[0]

# check again
script_dir = os.path.dirname(os.path.abspath(__file__))
json_path = os.path.normpath(os.path.join(script_dir, "..", "database", "data.json"))
session_path = os.path.normpath(os.path.join(script_dir, "..", "database", "sessions.json"))

if os.path.exists(json_path):
    try:
        with open(json_path, "r") as f:
            content = f.read().strip()

            if content:
                data = json.loads(content)
            else:
                data = {"users": []}

    except json.JSONDecodeError:
        data = {"users": []}
else:
    data = {"users": []}
    
found = False 
for user in data["users"]:
    if user["email"] == email and user["password"] == password:
        found = True
        break

if found:
    session_id = secrets.token_hex(32)

    expires = int(time.time()) + EXPIRESESSION

    sessions = {}

    if os.path.exists(session_path):
        try:
            with open(session_path, "r") as f:
                sessions = json.load(f)
        except json.JSONDecodeError:
            sessions = {}

    sessions[session_id] = {
        "email": email,
        "expires": expires
    }

    with open(session_path, "w") as f:
        json.dump(sessions, f, indent=4)

    print("Status: 302\r\n", end='')
    print("Set-Cookie: session_id=" + session_id + "; Path=/; HttpOnly\r\n", end='')
    print("Location: /dashboard/\r\n\r\n", end='')
else:
    print("Status: 302\r\n", end='')
    print("Location: /error/\r\n\r\n", end='')