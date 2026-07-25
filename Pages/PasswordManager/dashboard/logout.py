#!/usr/bin/env python3

import os
import json

print("Content-Type: text/html\r\n", end='')

script_dir = os.path.dirname(os.path.abspath(__file__))
session_path = os.path.normpath(
    os.path.join(script_dir, "..", "database", "sessions.json")
)

session_id = None

# Get session_id from cookie
cookie_header = os.environ.get("HTTP_COOKIE", "")

for cookie in cookie_header.split(";"):
    cookie = cookie.strip()

    if cookie.startswith("session_id="):
        session_id = cookie.split("=", 1)[1]
        break


# Load sessions
sessions = {}

if os.path.exists(session_path):
    try:
        with open(session_path, "r") as f:
            content = f.read().strip()

            if content:
                sessions = json.loads(content)

    except (json.JSONDecodeError, IOError):
        # If database is broken, continue with empty sessions
        sessions = {}


# Delete session
if session_id and session_id in sessions:
    del sessions[session_id]


# Save sessions back
try:
    with open(session_path, "w") as f:
        json.dump(sessions, f, indent=4)

except IOError:
    # Logging could go here
    pass


# Delete cookie and redirect
print("Status: 302 Found\r\n", end='')
print("Set-Cookie: session_id=deleted; Path=/; Max-Age=0; HttpOnly\r\n", end='')
print("Location: /login/\r\n", end='')
print("\r\n", end='')