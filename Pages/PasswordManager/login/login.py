#!/usr/bin/env python3
import sys
import os
import json
import urllib.parse

# webserv sends POST body via stdin
body = sys.stdin.read(int(os.environ.get('CONTENT_LENGTH', 0)))
fields = urllib.parse.parse_qs(body)

email    = fields.get('email', [''])[0]
password = fields.get('password', [''])[0]

# check again
script_dir = os.path.dirname(os.path.abspath(__file__))
json_path = os.path.normpath(os.path.join(script_dir, "..", "database", "data.json"))

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
    print("Status: 302\r\n", end='')
    print("Location: /dashboard/\r\n\r\n", end='')
else:
    print("Status: 302\r\n", end='')
    print("Location: /error/\r\n\r\n", end='')