#!/usr/bin/env python3
import sys
import os
import json
import urllib.parse

# webserv sends POST body via stdin (same convention as login.py)
body = sys.stdin.read(int(os.environ.get('CONTENT_LENGTH', 0)))
fields = urllib.parse.parse_qs(body)

firstName = fields.get('firstName', [''])[0]
lastName  = fields.get('lastName', [''])[0]
email     = fields.get('email', [''])[0]
password  = fields.get('password', [''])[0]
avatar    = fields.get('avatar', [''])[0]   # filename returned by upload.py

script_dir = os.path.dirname(os.path.abspath(__file__))

# login.py reads its data.json from ITS OWN folder (../login/, based on
# the "../login/" link in index.html). For login to find users created
# here, we have to write to that SAME file rather than one sitting next
# to this script. Adjust this path if your real folder layout differs.
json_path = os.path.normpath(os.path.join(script_dir, "..", "database", "data.json"))

if os.path.exists(json_path):
    with open(json_path, "r") as f:
        data = json.load(f)
else:
    data = {"users": []}

for user in data["users"]:
    if user["email"] == email:
        print("Status: 409\r\n", end="")
        print("Content-Type: text/plain\r\n\r\n", end="")
        print("E-Mail bereits registriert.", end="")
        sys.exit()

new_user = {
    "firstName": firstName,
    "lastName": lastName,
    "email": email,
    "password": password,
    "profilePicture": avatar
}

data["users"].append(new_user)

with open(json_path, "w") as f:
    json.dump(data, f, indent=4)

print("Status: 200\r\n", end="")
print("Content-Type: text/plain\r\n\r\n", end="")
print("Registrierung erfolgreich.", end="")
