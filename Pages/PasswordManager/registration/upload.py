#!/usr/bin/env python3
import sys
import os
import time

def read_exact(stream, length):
    data = bytearray()
    while len(data) < length:
        chunk = stream.read(length - len(data))
        if not chunk:
            # EOF before we got everything expected — stop, don't spin forever
            break
        data.extend(chunk)
    return bytes(data)

content_length = int(os.environ.get('CONTENT_LENGTH', 0))
content_type = os.environ.get('CONTENT_TYPE', 'application/octet-stream')
# ✅ Guard: if no body, reject
# 
# Binary mode is required here — sys.stdin.read() decodes as text and
# will corrupt raw image bytes. login.py/register.py can use text mode
# because their bodies are url-encoded form data, not binary.
raw_bytes = read_exact(sys.stdin.buffer, content_length)
with open("/tmp/upload_debug.log", "a") as dbg:
    dbg.write(f"CONTENT_LENGTH={content_length} actual_read={len(raw_bytes)}\n")
    
ext_map = {
    'image/jpeg': '.jpg',
    'image/png': '.png',
    'image/gif': '.gif',
    'image/webp': '.webp',
}
ext = ext_map.get(content_type, '.bin')

# Unique filename so two uploads never collide/overwrite each other
filename = f"{int(time.time() * 1000)}-{os.urandom(4).hex()}{ext}"

script_dir = os.path.dirname(os.path.abspath(__file__))
uploads_dir = os.path.join(script_dir, '..', 'database', 'uploads')
os.makedirs(uploads_dir, exist_ok=True)

with open(os.path.join(uploads_dir, filename), 'wb') as f:
    f.write(raw_bytes)

print("Status: 200\r\n", end='')
print("Content-Type: text/plain\r\n\r\n", end='')
print(filename, end='')
