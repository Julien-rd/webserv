#!/usr/bin/env python3
import os
import json
import time
import sys

script_dir = os.path.dirname(os.path.abspath(__file__))
session_path = os.path.normpath(os.path.join(script_dir, "..", "database", "sessions.json"))
data_path = os.path.normpath(os.path.join(script_dir, "..", "database", "data.json"))

def send_response(status_line, body_dict):
    body = json.dumps(body_dict)
    headers = [
        "Content-Type: application/json",
    ]
    if status_line:
        headers.insert(0, f"Status: {status_line}")

    # Header-Zeilen mit \r\n verbinden, dann GENAU EIN \r\n\r\n als Trenner
    sys.stdout.write("\r\n".join(headers) + "\r\n\r\n" + body)
    sys.exit()

cookie_header = os.environ.get("HTTP_COOKIE", "")
session_id = None
for cookie in cookie_header.split(";"):
    cookie = cookie.strip()
    if cookie.startswith("session_id="):
        session_id = cookie.split("=", 1)[1]
        break

if not session_id:
    send_response("401", {"error": "not logged in"})

sessions = {}
try:
    if os.path.exists(session_path):
        with open(session_path, "r") as f:
            sessions = json.load(f)
except (json.JSONDecodeError, IOError):
    sessions = {}

session = sessions.get(session_id)
if not session:
    send_response("401", {"error": "invalid session"})
    exit()

expires = session.get("expires")
if not expires or expires <= time.time():
    send_response("401", {"error": "expired session"})

data = {}
try:
    if os.path.exists(data_path):
        with open(data_path, "r") as f:
            data = json.load(f)
except (json.JSONDecodeError, IOError):
    data = {}

for user in data.get("users", []):
    if user["email"] == session["email"]:
        safe_user = {
            "firstName": user.get("firstName", ""),
            "lastName": user.get("lastName", ""),
            "email": user.get("email", ""),
            "profilePicture": user.get("profilePicture", ""),
        }
        send_response(None, safe_user)

send_response("401", {"error": "user not found"})