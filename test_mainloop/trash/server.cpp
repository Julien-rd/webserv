#include <cstring>
#include <iostream>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <poll.h>
#include <vector>
#include <sys/epoll.h>

using namespace std;

void set_socket(vector<struct pollfd> &open_sockets, int socket_fd)
{
    struct pollfd new_socket;
    memset(&new_socket, 0, sizeof(new_socket));
    new_socket.fd = socket_fd;
    new_socket.events = POLLIN;
    open_sockets.push_back(new_socket);
}

void rm_socket(vector<struct pollfd> &open_sockets, size_t &pos)
{
    close(open_sockets[pos].fd);
    open_sockets[pos] = open_sockets.back();
    open_sockets.pop_back();
    --pos;
}

int main()
{
    int serverSocket = socket(AF_INET, SOCK_STREAM, 0);

    int opt = 1;
    setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    int epfd = epoll_create(0);

    struct epoll_event ev;
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    ev.events = EPOLLIN;
    ev.data.fd = sock;

    sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(8080);
    serverAddress.sin_addr.s_addr = INADDR_ANY;

    bind(serverSocket, (struct sockaddr *)&serverAddress, sizeof(serverAddress));
    listen(serverSocket, 5);
    int time = 0;
    vector<struct pollfd> open_sockets;
    set_socket(open_sockets, serverSocket);

    int epfd = epoll_create1(0);
    epoll_ctl(epfd, EPOLL_CTL_ADD, sock, &event);
    epoll_wait(epfd, events, maxevents, timeout);

    while (1)
    {
        if (poll(open_sockets.data(), open_sockets.size(), -1) == -1)
        {
            perror("poll error");
            break;
        }
        for (size_t i = 0; i < open_sockets.size(); ++i)
        {
            if (open_sockets[i].revents == 0)
                continue;
            if (open_sockets[i].fd == serverSocket)
            {
                if (open_sockets[i].revents & POLLIN)
                {
                    int clientSocket = accept(serverSocket, nullptr, nullptr);
                    std::cout << "new client ID : " << clientSocket << std::endl;
                    set_socket(open_sockets, clientSocket);
                }
            }
            else if (open_sockets[i].revents & (POLLHUP | POLLERR | POLLNVAL))
            {
                cout << "Client ID: error/hangup " << open_sockets[i].fd << endl;
                rm_socket(open_sockets, i);
            }
            else if (open_sockets[i].revents & POLLIN)
            {
                char buffer[1024] = {0};
                ssize_t ret_val;
                std::cout << "client ID trying to connect : " << open_sockets[i].fd << std::endl;
                ret_val = recv(open_sockets[i].fd, buffer, sizeof(buffer), 0);
                if (ret_val == 0)
                {
                    cout << "client " << open_sockets[i].fd << " :hung up" << endl;
                    rm_socket(open_sockets, i);
                }
                else if (ret_val == -1)
                {
                    cout << "error recv" << endl;
                    rm_socket(open_sockets, i);
                }
                else
                    cout << "Message from client: " << buffer << endl;
            }
            else
                ++time;
        }
    }
    close(serverSocket);
}
