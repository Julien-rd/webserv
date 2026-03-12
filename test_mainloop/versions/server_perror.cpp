#include <csignal>
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <netinet/in.h>
#include <poll.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>
#include <vector>
#include <unordered_set>

using namespace std;

#define MAX_CLIENTS 1024 // wird im config file festgelegt, nginx macht es anscheinend so, dass die fds in einem array gespeichert werden, das so gro ist, wie die max Clients

volatile int gSignalStatus = 0;

void signal_handler(int signal) { gSignalStatus = signal; }

void cleanup(int open_fds[MAX_CLIENTS])
{
    size_t iter = 3;
    while(iter < MAX_CLIENTS){
        if(open_fds[iter] != -1)
            close(open_fds[iter]);
        ++iter;
    }
}

struct ErrorFlag
{
    enum Type
    {
        ERR_EPOLL_WAIT,
        ERR_SOCKET,
        ERR_ACCEPT,
        ERR_FCNTL,
        ERR_SETSOCKOPT,
        ERR_EPOLL_CREATE1,
        ERR_EPOLL_CTL,
        ERR_BIND,
        ERR_LISTEN,
        ERR_READ
    };
};

int error_msg(ErrorFlag::Type type, int open_fds[MAX_CLIENTS])
{
    int ret = 1;
    switch (type)
    {
    case ErrorFlag::ERR_EPOLL_WAIT:
        perror("epoll_wait");
        break;
    case ErrorFlag::ERR_SOCKET:
        perror("socket");
        break;
    case ErrorFlag::ERR_ACCEPT:
        perror("accept");
        break;
    case ErrorFlag::ERR_FCNTL:
        perror("fcntl");
        break;
    case ErrorFlag::ERR_SETSOCKOPT:
        perror("setsockopt");
        break;
    case ErrorFlag::ERR_EPOLL_CREATE1:
        perror("epoll_create1");
        break;
    case ErrorFlag::ERR_EPOLL_CTL:
        perror("epoll_ctl");
        break;
    case ErrorFlag::ERR_BIND:
        perror("bind");
        break;
    case ErrorFlag::ERR_LISTEN:
        perror("listen");
        break;
    case ErrorFlag::ERR_READ:
        if (errno == EAGAIN || errno == EWOULDBLOCK)
            ret = 0;
        else
            perror("read");
        break;
    }
if(ret== 1)
   cleanup(open_fds);
return ret;
}

sockaddr_in set_sockaddr()
{
    sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(8080);
    serverAddress.sin_addr.s_addr = INADDR_ANY;
    return serverAddress;
}

int add_socket(int socket_fd, int epfd)
{
    struct epoll_event ev;
    ev.events = EPOLLIN | EPOLLET;
    ev.data.fd = socket_fd;
    if (epoll_ctl(epfd, EPOLL_CTL_ADD, socket_fd, &ev) == -1)
        return 1;
    return 0;
}

bool set_nonblocking(int fd)
{
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1)
        return false;
    flags = flags | O_NONBLOCK;
    if (fcntl(fd, F_SETFL, flags) == -1)
        return false;
    return true;
}

void pop_socket(std::unordered_set<int> &open_fds, int fd){
    close(fd);
    open_fds.erase(fd);
}

void set_arr(int open_fds[MAX_CLIENTS]){
    size_t iter = 0;
    while(iter < MAX_CLIENTS){
        open_fds[iter] = -1;
        ++iter;
    }
}

int main()
{
    std::signal(SIGINT, signal_handler);
    int open_fds[MAX_CLIENTS]; //vorteil zu unordered set: bei insert kann es zu rehashing kommen was theoretisch O(n) dauern kann, und muesste eig immer funktionieren da linux fd sequenziell freigibt
    // std::unordered_set<int> open_fds;
    int serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket == -1)
        return error_msg(ErrorFlag::ERR_SOCKET, open_fds);
    open_fds[serverSocket] = serverSocket;
    if (!set_nonblocking(serverSocket))
        return error_msg(ErrorFlag::ERR_FCNTL, open_fds);
    int opt = 1;
    if (setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) ==
        -1)
        return error_msg(ErrorFlag::ERR_SETSOCKOPT, open_fds);

    sockaddr_in serverAddress = set_sockaddr();

    int epfd = epoll_create1(0);
    if (epfd == -1)
        return error_msg(ErrorFlag::ERR_EPOLL_CREATE1, open_fds);
    open_fds[epfd] = epfd;

    if (add_socket(serverSocket, epfd) == -1)
        return error_msg(ErrorFlag::ERR_EPOLL_CTL, open_fds);
    if (bind(serverSocket, (struct sockaddr *)&serverAddress,
             sizeof(serverAddress)) == -1)
        return (ErrorFlag::ERR_BIND);
    if (listen(serverSocket, 5) == -1)
        return (ErrorFlag::ERR_LISTEN);

    std::vector<epoll_event> request_buf(MAX_CLIENTS);
    while (1)
    {
        std::cout << "waiting for request \n";
        int ready_events = epoll_wait(epfd, request_buf.data(), MAX_CLIENTS, -1);
        if (gSignalStatus)
            break;
        if (ready_events == -1)
            return error_msg(ErrorFlag::ERR_EPOLL_WAIT, open_fds);
        for (int i = 0; i < ready_events; i++)
        {
            int fd = request_buf[i].data.fd;
            if (fd == serverSocket)
            {
                while (true)
                {
                    int ClientSocket = accept(serverSocket, nullptr, nullptr);
                    if (ClientSocket == -1)
                    {
                        if (errno == EAGAIN || errno == EWOULDBLOCK)
                            break;
                        else
                            return error_msg(ErrorFlag::ERR_ACCEPT, open_fds);
                        break;
                    }
                    open_fds[ClientSocket] = ClientSocket;
                    if (set_nonblocking(ClientSocket) == false)
                        return error_msg(ErrorFlag::ERR_FCNTL, open_fds);
                    if (add_socket(ClientSocket, epfd))
                        return error_msg(ErrorFlag::ERR_EPOLL_CTL, open_fds);
                    std::cout << "Client accepted: FD " << ClientSocket << "\n";
                }
            }
            else
            {
                std::cout << "message from client FD " << fd << " received!\n";
                char buffer[10];
                while (1)
                {
                    ssize_t bytes_read = read(fd, buffer, 10);
                    if (bytes_read == 0)
                    {
                        std::cout << "client FD " << fd << " closed connection!\n";
                        if (epoll_ctl(epfd, EPOLL_CTL_DEL, fd, nullptr) == -1)
                            return error_msg(ErrorFlag::ERR_EPOLL_CTL, open_fds);
                        open_fds[fd] = -1;
                        if (close(fd) == -1)
                            return error_msg(ErrorFlag::ERR_READ, open_fds);
                        break;
                    }
                    if (bytes_read == -1)
                    {
                        if (error_msg(ErrorFlag::ERR_READ, open_fds) == 1)
                            return 1;
                        else{
                            break ;
                        }
                    }
                    buffer[bytes_read] = 0;
                    std::cout << buffer;
                }
            }
        }
    }
    cleanup(open_fds);
}
