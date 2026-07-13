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
with open(os.path.join(script_dir, 'data.json'), 'r') as f:
    data = json.load(f)
    
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