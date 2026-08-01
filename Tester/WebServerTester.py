#!/usr/bin/env python3
"""
Black-box test suite for a webserver.

Talks to the server over raw TCP sockets only (no `requests` library),
so malformed / chunked / partial input reaches the server exactly as sent.

Usage:
    python3 webserver_test.py --host 127.0.0.1 --port 8080
    python3 webserver_test.py --fast          # skip the slow/heavy tests

Structure:
    - TESTS is a list of (category, name, fn) filled in by the @test decorator.
    - Each test is a small function that raises AssertionError on failure.
    - Destructive tests (resource exhaustion, fault injection, fuzz) always
      finish with assert_server_survived() so a crash is caught immediately
      instead of causing a wall of confusing timeouts in later tests.
"""

import argparse
import random
import socket
import struct
import sys
import threading
import time

# ===================== Config (overridden by CLI args) =====================

HOST = "127.0.0.1"
PORT = 7070
FAST = False  # --fast skips heavy/slow tests

CONCURRENT_CLIENTS = 100
HOLD_SECONDS = 3
FD_EXHAUSTION_CONNECTIONS = 500
ABRUPT_DISCONNECT_COUNT = 100
FUZZ_CLIENT_COUNT = 50

# ===================== Test registry =====================

TESTS = []  # list of (category, name, fn)


def test(category, name):
    """Decorator: registers a function as a named test under a category."""
    def wrapper(fn):
        TESTS.append((category, name, fn))
        return fn
    return wrapper


def heavy(fn):
    """Mark a test as skippable with --fast."""
    fn._heavy = True
    return fn


# ===================== Core helpers =====================

def raw_request(data: bytes, read_timeout: float = 2.0) -> bytes:
    """Open a connection, send raw bytes, read until close/timeout, return raw response."""
    with socket.create_connection((HOST, PORT), timeout=read_timeout) as s:
        s.sendall(data)
        s.settimeout(read_timeout)
        chunks = []
        try:
            while True:
                chunk = s.recv(4096)
                if not chunk:
                    break
                chunks.append(chunk)
        except socket.timeout:
            pass
        return b"".join(chunks)


def rst_close(s: socket.socket):
    """Close a socket by sending a hard TCP RST instead of a clean FIN.
    Forces the peer's next send()/recv() on this connection to hit a real error
    (ECONNRESET / EPIPE), instead of a graceful close."""
    s.setsockopt(socket.SOL_SOCKET, socket.SO_LINGER, struct.pack("ii", 1, 0))
    s.close()


def is_server_alive(timeout: float = 2.0) -> bool:
    try:
        resp = raw_request(b"GET / HTTP/1.1\r\nHost: localhost\r\n\r\n", read_timeout=timeout)
        return resp.startswith(b"HTTP/1.")
    except Exception:
        return False


def assert_server_survived():
    """Call after anything destructive. Fails loudly and specifically if the
    server died, instead of letting later tests fail with confusing timeouts."""
    time.sleep(0.3)  # give the server a moment to recover / free resources
    assert is_server_alive(), "Server did not respond afterwards — it may have crashed"


# ===================== Protocol Basics =====================

@test("Protocol Basics", "GET / returns a valid HTTP status line")
def _():
    resp = raw_request(b"GET / HTTP/1.1\r\nHost: localhost\r\n\r\n")
    assert resp.startswith(b"HTTP/1."), resp[:80]


@test("Protocol Basics", "Unsupported method does not crash the server")
def _():
    resp = raw_request(b"FOOBAR / HTTP/1.1\r\nHost: localhost\r\n\r\n")
    assert resp.startswith(b"HTTP/1.")
    assert_server_survived()


@test("Protocol Basics", "HTTP/1.1 without Host header returns 400")
def _():
    resp = raw_request(b"GET / HTTP/1.1\r\n\r\n")
    status_line = resp.split(b"\r\n", 1)[0]
    assert b"400" in status_line, status_line


@test("Protocol Basics", "HTTP/1.0 request is handled")
def _():
    resp = raw_request(b"GET / HTTP/1.0\r\n\r\n")
    assert resp.startswith(b"HTTP/1.")


@test("Protocol Basics", "Duplicate headers do not crash the server")
def _():
    resp = raw_request(
        b"GET / HTTP/1.1\r\nHost: localhost\r\nHost: evil\r\n\r\n"
    )
    assert resp == b"" or resp.startswith(b"HTTP/1.")
    assert_server_survived()


# ===================== Chunked Encoding =====================

@test("Chunked Encoding", "Well-formed chunked POST is accepted")
def _():
    body = b"4\r\nWiki\r\n5\r\npedia\r\n0\r\n\r\n"
    req = (b"POST /login HTTP/1.1\r\nHost: localhost\r\n"
           b"Transfer-Encoding: chunked\r\n\r\n" + body)
    resp = raw_request(req)
    assert resp.startswith(b"HTTP/1.0") or resp.startswith(b"HTTP/1.1 2"), resp[:80]


@test("Chunked Encoding", "Malformed chunk size does not crash the server")
def _():
    bad_body = b"ZZZ\r\nWikipedia\r\n0\r\n\r\n"  # not valid hex
    req = (b"POST /upload HTTP/1.1\r\nHost: localhost\r\n"
           b"Transfer-Encoding: chunked\r\n\r\n" + bad_body)
    try:
        resp = raw_request(req, read_timeout=2)
        assert resp == b"" or resp.startswith(b"HTTP/1.")
    except (socket.timeout, ConnectionResetError, BrokenPipeError):
        pass
    assert_server_survived()


# ===================== Concurrency =====================

@heavy
@test("Concurrency", f"{CONCURRENT_CLIENTS} concurrent clients, basic GET")
def _():
    results = []

    def client():
        try:
            resp = raw_request(b"GET / HTTP/1.1\r\nHost: localhost\r\n\r\n")
            results.append(resp.startswith(b"HTTP/1."))
        except Exception:
            results.append(False)

    threads = [threading.Thread(target=client) for _ in range(CONCURRENT_CLIENTS)]
    for t in threads:
        t.start()
    for t in threads:
        t.join()

    failures = results.count(False)
    assert failures == 0, f"{failures}/{len(results)} clients failed"


@heavy
@test("Concurrency", f"{CONCURRENT_CLIENTS} clients holding connection {HOLD_SECONDS}s")
def _():
    results = []

    def client():
        try:
            with socket.create_connection((HOST, PORT), timeout=HOLD_SECONDS + 5) as s:
                s.sendall(b"GET / HTTP/1.1\r\nHost: localhost\r\nConnection: keep-alive\r\n\r\n")
                time.sleep(HOLD_SECONDS)
                s.settimeout(3)
                data = s.recv(4096)
                results.append(data.startswith(b"HTTP/1."))
        except Exception:
            results.append(False)

    threads = [threading.Thread(target=client) for _ in range(CONCURRENT_CLIENTS)]
    for t in threads:
        t.start()
    for t in threads:
        t.join()

    failures = results.count(False)
    assert failures == 0, f"{failures}/{len(results)} clients failed"


@heavy
@test("Concurrency", "Chunked requests from concurrent clients")
def _():
    body = b"4\r\nWiki\r\n5\r\npedia\r\n0\r\n\r\n"
    req = (b"POST /upload HTTP/1.1\r\nHost: localhost\r\n"
           b"Transfer-Encoding: chunked\r\n\r\n" + body)
    results = []

    def client():
        try:
            resp = raw_request(req)
            results.append(resp.startswith(b"HTTP/1."))
        except Exception:
            results.append(False)

    threads = [threading.Thread(target=client) for _ in range(20)]
    for t in threads:
        t.start()
    for t in threads:
        t.join()

    failures = results.count(False)
    assert failures == 0, f"{failures}/{len(results)} clients failed"


# ===================== Slow Clients =====================

@test("Slow Clients", "Slowloris-style byte-by-byte request")
def _():
    s = socket.create_connection((HOST, PORT), timeout=5)
    try:
        request = b"GET / HTTP/1.1\r\nHost: localhost\r\n\r\n"
        for b in request:
            s.sendall(bytes([b]))
            time.sleep(0.03)
        s.settimeout(3)
        resp = s.recv(4096)
        assert resp.startswith(b"HTTP/1.")
    finally:
        s.close()


# ===================== Resource Exhaustion =====================

@heavy
@test("Resource Exhaustion", f"Open {FD_EXHAUSTION_CONNECTIONS} connections without closing")
def _():
    socks = []
    try:
        for _ in range(FD_EXHAUSTION_CONNECTIONS):
            try:
                s = socket.create_connection((HOST, PORT), timeout=2)
                s.sendall(b"GET / HTTP/1.1\r\nHost: localhost\r\nConnection: keep-alive\r\n\r\n")
                socks.append(s)
            except OSError:
                # Server legitimately refusing new connections once resources
                # are tight is acceptable — it must not crash, though.
                break
    finally:
        for s in socks:
            try:
                s.close()
            except Exception:
                pass
    assert_server_survived()


# ===================== Fault Injection (black-box) =====================
# We can't call the server's send()/recv() directly, but a TCP RST forces a
# *real* send/recv error on the server's side of an in-flight connection —
# this is a genuine fault, not a simulated one.

@heavy
@test("Fault Injection", f"Abrupt RST mid-request x{ABRUPT_DISCONNECT_COUNT}")
def _():
    def abrupt_client():
        try:
            s = socket.create_connection((HOST, PORT), timeout=2)
            s.sendall(b"GET / HTTP/1.1\r\nHost: l")  # partial, incomplete request
            rst_close(s)
        except Exception:
            pass

    threads = [threading.Thread(target=abrupt_client) for _ in range(ABRUPT_DISCONNECT_COUNT)]
    for t in threads:
        t.start()
    for t in threads:
        t.join()

    assert_server_survived()


@heavy
@test("Fault Injection", "Client disconnects before reading response (RST after full request)")
def _():
    def client():
        try:
            s = socket.create_connection((HOST, PORT), timeout=2)
            s.sendall(b"GET / HTTP/1.1\r\nHost: localhost\r\n\r\n")
            rst_close(s)  # server's send() of the response should now fail
        except Exception:
            pass

    threads = [threading.Thread(target=client) for _ in range(ABRUPT_DISCONNECT_COUNT)]
    for t in threads:
        t.start()
    for t in threads:
        t.join()

    assert_server_survived()


# ===================== Stress / Fuzz =====================

@test("Stress / Fuzz", "Oversized header does not crash the server")
def _():
    huge_value = b"A" * (1024 * 1024)  # 1MB header value
    req = b"GET / HTTP/1.1\r\nHost: localhost\r\nX-Test: " + huge_value + b"\r\n\r\n"
    try:
        resp = raw_request(req, read_timeout=3)
        assert resp == b"" or resp.startswith(b"HTTP/1.")
    except (socket.timeout, ConnectionResetError, BrokenPipeError):
        pass
    assert_server_survived()


@heavy
@test("Stress / Fuzz", f"Random garbage bytes x{FUZZ_CLIENT_COUNT} concurrent")
def _():
    def fuzz_client():
        try:
            s = socket.create_connection((HOST, PORT), timeout=2)
            garbage = bytes(random.getrandbits(8) for _ in range(200))
            s.sendall(garbage)
            s.settimeout(1)
            try:
                s.recv(4096)
            except Exception:
                pass
            s.close()
        except Exception:
            pass

    threads = [threading.Thread(target=fuzz_client) for _ in range(FUZZ_CLIENT_COUNT)]
    for t in threads:
        t.start()
    for t in threads:
        t.join()

    assert_server_survived()


# ===================== Runner =====================

def run_all():
    passed, failed, skipped = 0, 0, 0
    current_category = None
    for category, name, fn in TESTS:
        if FAST and getattr(fn, "_heavy", False):
            skipped += 1
            continue
        if category != current_category:
            print(f"\n=== {category} ===")
            current_category = category
        try:
            fn()
            print(f"  {name}: OK")
            passed += 1
        except AssertionError as e:
            print(f"  {name}: FAIL ({e})")
            failed += 1
        except Exception as e:
            print(f"  {name}: FAIL (unexpected error: {type(e).__name__}: {e})")
            failed += 1

    print(f"\n{passed} passed, {failed} failed, {skipped} skipped")
    return failed == 0


def main():
    global HOST, PORT, FAST
    parser = argparse.ArgumentParser(description="Black-box test suite for a webserver")
    parser.add_argument("--host", default=HOST)
    parser.add_argument("--port", type=int, default=PORT)
    parser.add_argument("--fast", action="store_true", help="skip slow/heavy tests")
    args = parser.parse_args()

    HOST, PORT, FAST = args.host, args.port, args.fast

    print(f"Target: {HOST}:{PORT}")
    if not is_server_alive():
        print("ERROR: server is not responding before tests even started. Is it running?")
        sys.exit(1)

    ok = run_all()
    sys.exit(0 if ok else 1)


if __name__ == "__main__":
    main()