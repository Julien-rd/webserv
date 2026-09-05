import socket
s = socket.create_connection(("127.0.0.1", 7070))
s.sendall(b"GET /login/ HTTP/1.1\r\nHost: x\r\nBROKENHEADER\r\n\r\n")
print(s.recv(4096))