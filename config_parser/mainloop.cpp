
#include <cerrno>
#include <csignal>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <unistd.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/poll.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <poll.h>
#include <netdb.h>

volatile sig_atomic_t signals = 0;
void sigHandler(int num) {
    signals = 1;
}
int main() {
    signal(SIGINT, sigHandler);
   addrinfo hints;
   addrinfo *results;
   memset(&hints, 0, sizeof(hints));
   hints.ai_socktype = SOCK_STREAM;
   hints.ai_family = AF_INET;
   int res = getaddrinfo("0.0.0.0", "8080", &hints, &results);
   int sock = socket(AF_INET, SOCK_STREAM , 0);
   int bind_fd = bind(sock, results->ai_addr, results->ai_addrlen);
   listen(sock, 6);
   sockaddr_in clientSock;
   socklen_t    clientLen = sizeof(clientSock);
   
   int client = 0;
   pollfd fds[10];
   fds[0].fd = sock;
   fds[0].events = POLLIN;
   int nfds = 1;
   char buf[4096];
   while (1) {
       if (signals == 1) {
           freeaddrinfo(results);
           close(sock);
           for(int i = 0; i < nfds; ++i) {
               if (fds[i].fd != -1)
                   close(fds[i].fd);
           }
           std::cout << "signal caught\n";
           exit(1);
       }
       int incoming = poll(fds, nfds, -1);
       if (incoming > 0){
           for (nfds_t i = 0; i < nfds; ++i) {
               if (fds[i].fd == -1)
                   continue;
               if (fds[i].revents & POLLIN && i == 0) {
                   client = accept(sock, (sockaddr *) &clientSock, &clientLen);
                   std::cout << client << std::endl;
                   fds[nfds].fd = client;
                   fds[nfds].events = POLLIN;
                   ++nfds;
                   continue;
               }
               else if (fds[i].revents & POLLIN) {
                   int m = 1;
                   int flags = fcntl(fds[i].fd, F_GETFL, 0);
                   fcntl(fds[i].fd, F_SETFL, flags | O_NONBLOCK);
                   while (m > 0) {
                        m = recv(fds[i].fd, (void *)buf, sizeof(buf), 0);
                        if (m == -1)
                            break;
                        buf[m] = 0;
                        if (m == 0) {
                            std::cout << "client disconnected\n";
                            close(fds[i].fd);
                            fds[i].fd = -1;
                            fds[i].revents = 0;
                        }
                        std::cout << buf << std::endl;
                   }
                   if (m == -1 && errno != EAGAIN && errno != EWOULDBLOCK)
                       exit(1);
                   memset(buf, 0,  4096);
               }
               else if (fds[i].revents & POLLHUP || fds[i].revents & POLLERR|| fds[i].revents & POLLNVAL) {
                   std::cout << "client disconnected\n";
                   close(fds[i].fd);
                   fds[i].fd = -1;
               }
           }

       }
   }
}
