#include <csignal>
#include <fcntl.h>
#include <iostream>
#include <cstring>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

volatile sig_atomic_t signals = 0;
void sigHandler(int num) {
    signals = 1;
}
int main() {
    signal(SIGINT, sigHandler);
    const char* ip = "127.0.0.1";
    const int port = 8080;

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        std::cerr << "socket failed\n";
        return 1;
    }

    sockaddr_in server{};
    server.sin_family = AF_INET;
    server.sin_port = htons(port);

    if (inet_pton(AF_INET, ip, &server.sin_addr) <= 0) {
        std::cerr << "invalid address\n";
        return 1;
    }

    if (connect(sock, (sockaddr*)&server, sizeof(server)) < 0) {
        std::cerr << "connect failed\n";
        return 1;
    }

    std::cout << "connected to " << ip << ":" << port << "\n";
    char buf[4096];
    // send/receive loop
    std::string msg;
    while (true) {
        if (signals == 1) {
            std::cout << "interrupt called\n";
            close(sock);
            exit(1);
        }
        int flags = fcntl(sock, F_GETFL, 0);
        fcntl(sock, F_SETFL, flags | O_NONBLOCK);
        if (recv(sock, buf, sizeof(buf), 0) == 0) {
            std::cout << "server closed\n";
            close(sock);
            exit(0);
        }
        std::cout << "send: ";
        std::getline(std::cin, msg);
        if (msg == "quit") break;
        int ret = send(sock, msg.c_str(), msg.size(), 0);
        if (ret < 0) {
            close(sock);
            exit(0);
        }
    }

    close(sock);
    return 0;
}