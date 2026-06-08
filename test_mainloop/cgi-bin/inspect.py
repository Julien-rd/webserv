#!/usr/bin/env python3
import cgi
import cgitb
import os
import sys

method = os.environ.get("REQUEST_METHOD", "GET")
query_string = os.environ.get("QUERY_STRING", "")

form = cgi.FieldStorage()

print("Content-Type: text/html\r\n\r\n", end="")

print("""<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>CGI Inspector</title>
  <style>
    * { box-sizing: border-box; margin: 0; padding: 0; }
    body {
      font-family: 'SF Mono', 'Fira Code', 'Cascadia Code', monospace;
      background: #0d1117;
      color: #e6edf3;
      min-height: 100vh;
      padding: 2rem;
    }
    .container { max-width: 760px; margin: 0 auto; }
    header {
      border-bottom: 1px solid #30363d;
      padding-bottom: 1.25rem;
      margin-bottom: 2rem;
    }
    .badge {
      display: inline-block;
      padding: 3px 10px;
      border-radius: 4px;
      font-size: 12px;
      font-weight: 600;
      letter-spacing: 0.08em;
      margin-bottom: 0.75rem;
    }
    .badge-get  { background: #1c4a2e; color: #3fb950; border: 1px solid #238636; }
    .badge-post { background: #3d2a00; color: #d29922; border: 1px solid #9e6a03; }
    h1 { font-size: 1.1rem; color: #c9d1d9; font-weight: 500; }
    h1 span { color: #58a6ff; }
    section { margin-bottom: 2rem; }
    h2 {
      font-size: 11px;
      text-transform: uppercase;
      letter-spacing: 0.12em;
      color: #8b949e;
      margin-bottom: 0.75rem;
      padding-bottom: 0.4rem;
      border-bottom: 1px solid #21262d;
    }
    table {
      width: 100%;
      border-collapse: collapse;
      font-size: 13px;
    }
    tr { border-bottom: 1px solid #21262d; }
    tr:last-child { border-bottom: none; }
    td { padding: 7px 10px; vertical-align: top; }
    td:first-child {
      color: #79c0ff;
      width: 42%;
      word-break: break-all;
    }
    td:last-child {
      color: #e6edf3;
      word-break: break-all;
    }
    .empty { color: #484f58; font-style: italic; font-size: 12px; padding: 0.5rem 0; }
    .highlight td { background: #161b22; }
    .highlight td:first-child { color: #d2a8ff; }
    .tag-form { color: #f0883e; }
  </style>
</head>
<body>
<div class="container">
  <header>""")

if method == "POST":
    print('<span class="badge badge-post">POST</span>')
else:
    print('<span class="badge badge-get">GET</span>')

print(f'    <h1>CGI Inspector &mdash; <span>{os.environ.get("SCRIPT_NAME", "/cgi-bin/inspect.py")}</span></h1>')
print("  </header>")

# --- Form fields section ---
print('  <section>')
print('    <h2><span class="tag-form">&#9632;</span> Form fields</h2>')

keys = list(form.keys())
if keys:
    print('    <table>')
    for key in keys:
        value = form.getvalue(key, "")
        print(f'      <tr class="highlight"><td>{cgi.escape(key)}</td><td>{cgi.escape(str(value))}</td></tr>')
    print('    </table>')
else:
    print('    <p class="empty">no fields submitted</p>')

print('  </section>')

# --- Query string section ---
print('  <section>')
print('    <h2>Query string</h2>')
if query_string:
    print(f'    <table><tr><td>raw</td><td>{cgi.escape(query_string)}</td></tr></table>')
    import urllib.parse
    parsed = urllib.parse.parse_qs(query_string, keep_blank_values=True)
    if parsed:
        print('    <table style="margin-top:8px">')
        for k, v in parsed.items():
            print(f'      <tr class="highlight"><td>{cgi.escape(k)}</td><td>{cgi.escape(", ".join(v))}</td></tr>')
        print('    </table>')
else:
    print('    <p class="empty">empty</p>')
print('  </section>')

# --- Selected CGI env vars ---
cgi_vars = [
    "REQUEST_METHOD", "SCRIPT_NAME", "PATH_INFO",
    "SERVER_NAME", "SERVER_PORT", "SERVER_PROTOCOL",
    "CONTENT_TYPE", "CONTENT_LENGTH",
    "HTTP_HOST", "HTTP_USER_AGENT", "REMOTE_ADDR",
]

print('  <section>')
print('    <h2>CGI environment</h2>')
print('    <table>')
for var in cgi_vars:
    val = os.environ.get(var, "")
    if val:
        print(f'      <tr><td>{var}</td><td>{cgi.escape(val)}</td></tr>')
print('    </table>')
print('  </section>')

print('</div></body></html>')
