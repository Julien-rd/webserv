#include "Server.hpp"

#include "../Error/Error.hpp"
#include "../Utils/Macros.hpp"

#include <exception>
#include <fcntl.h>
#include <iostream>
#include <netdb.h>
#include <sstream>
#include <string>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

Server::Server(t_serverContext context)
        : _config(context.config)
        , _epfd(context.epfd)
        , _sid(context.sid)
        , _serverToClientsMap(context.serverToClientsMap)
        , _clientToServerMap(context.clientToServerMap)
        , _clients(context.clients)
        , _serverSocket(-1)
        , _addrInfo(NULL) {}

Server::Server(const Server &obj)
        : _config(obj._config)
        , _epfd(obj._epfd)
        , _sid(obj._sid)
        , _serverToClientsMap(obj._serverToClientsMap)
        , _clientToServerMap(obj._clientToServerMap)
        , _clients(obj._clients)
        , _serverSocket(obj._serverSocket)
        , _addrInfo(NULL) {}

Server::~Server(void) {
    if (_addrInfo)
        freeaddrinfo(_addrInfo);
}

void Server::closeClientFds(void) {
    int fd;

    for (std::map<int, Client>::iterator it = _clients.begin(); it != _clients.end(); it++) {
        fd = it->second.getFd();
        if (fd != -1)
            close(fd);
    }
}

void Server::updateClientsMap(e_mapOperation op, const int clientFd) {
    switch (op) {
    case ADD: {
        _clientToServerMap.insert(std::pair<int, int>(clientFd, _serverSocket));
        Client &client = _clients[clientFd];
        client.init(_epfd, &_config, _sid, clientFd);
        _serverToClientsMap.at(_serverSocket).insert(clientFd);
        break;
    }
    case REMOVE:
        _clientToServerMap.erase(clientFd);
        _clients.at(clientFd).reset();
        _clients.erase(clientFd);
        _serverToClientsMap.at(_serverSocket).erase(clientFd);
    }
}

void Server::closeConnection(int clientFd) {
    int postFd = _clients.at(clientFd).getCGI().getPostFd();
    if (postFd != -1) {
        epoll_ctl(_epfd, EPOLL_CTL_DEL, postFd, NULL);
        close(postFd);
    }
    int readFd = _clients.at(clientFd).getCGI().getReadFd();
    if (readFd != -1) {
        epoll_ctl(_epfd, EPOLL_CTL_DEL, readFd, NULL);
        close(readFd);
    }
    updateClientsMap(REMOVE, clientFd);
    std::stringstream ss;
    ss << "Server " << _sid << " closed connection with Client " << clientFd;
    log(Level::INFO, ss.str());
    if (epoll_ctl(_epfd, EPOLL_CTL_DEL, clientFd, NULL) == -1) {
        error_msg(ERR_EPOLL_CTL);
        return;
    }
    if (close(clientFd) == -1)
        error_msg(ERR_CLOSE);
    return;
}

void Server::setToNonBlocking(int socketFd) {
    if (fcntl(socketFd, F_SETFL, FD_CLOEXEC | O_NONBLOCK) == -1)
        error_msg(ERR_FCNTL);  // Fix: file descriptor needs to be closed after this
}

void Server::initServerSocket(void) {
    _serverSocket = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0);
    if (_serverSocket == -1) {
        error_msg(ERR_SOCKET);
        throw std::exception();
    }
    int opt = 1;
    if (setsockopt(_serverSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1) {
        error_msg(ERR_SETSOCKOPT);
        throw std::exception();
    }
}

void Server::setServerSockAddr(void) {
    addrinfo hints = {0, 0, 0, 0, 0, 0, 0, 0};
    int      res;

    hints.ai_socktype = SOCK_STREAM;
    hints.ai_family = AF_UNSPEC;
    hints.ai_flags = AI_NUMERICHOST;
    res = getaddrinfo(
        _config.servers[_sid].ip.c_str(), _config.servers[_sid].port.c_str(), &hints, &_addrInfo);
    if (res) {
        // throw std::runtime_error(std::string("gettaddrinfo() failed: ") +
        //                          gai_strerror(res));
        throw std::exception();
    }
}

void Server::addSocketToEpoll(int socketFd) {
    struct epoll_event ev;
    ev.events = EPOLLIN;
    // ev.data.ptr = 0;
    ev.data.u64 = 0;
    ev.data.fd = socketFd;
    if (epoll_ctl(_epfd, EPOLL_CTL_ADD, socketFd, &ev) == -1) {
        error_msg(ERR_EPOLL_CTL);
        throw std::exception();
    }
}

void Server::bindAndListen(void) {
    if (bind(_serverSocket, _addrInfo->ai_addr, _addrInfo->ai_addrlen) == -1) {
        error_msg(ERR_BIND);
        throw std::exception();
    }
    if (listen(_serverSocket, SOMAXCONN) == -1) {  // TODO hardocded 20?
        error_msg(ERR_LISTEN);
        throw std::exception();
    }
}

int Server::start(void) {
    try {
        initServerSocket();
        setServerSockAddr();
        bindAndListen();
        addSocketToEpoll(_serverSocket);
    } catch (...) {
        if (_serverSocket != -1) {
            close(_serverSocket);
            _serverSocket = -1;
        }
        throw;
    }
    return _serverSocket;
}

int Server::checkClientCap(void) { return 0; }

void Server::newClient(int clientFd) {
    updateClientsMap(ADD, clientFd);
    setToNonBlocking(clientFd);
    addSocketToEpoll(clientFd);
    std::stringstream ss;
    ss << "Server " << _sid << " accepted a new Client " << clientFd;
    log(Level::INFO, ss.str());
}

void Server::handleServerEvent(void) {
    int clientFd;

    clientFd = accept(_serverSocket, NULL, NULL);
    if (clientFd == -1)
        return;
    if (checkClientCap() == ERR) {
        close(clientFd);
        return;
    }
    newClient(clientFd);
}

bool Server::recvClientEvent(Client &client) {
    std::string recvBuffer(BUFFER_SIZE, '\0');
    ssize_t     bytesRead = recv(client.getFd(), &recvBuffer[0], BUFFER_SIZE, 0);

    if (bytesRead <= 0) {
        if (bytesRead == -1)
            error_msg(ERR_RECV); //fix: log
        return false;
    }
    recvBuffer.resize(bytesRead);
    return client.parseRecvBuffer(recvBuffer);
}

void Server::handleClientEvent(int clientFd, unsigned int event) {
    Client &client = _clients.at(clientFd);

    if (event & (EPOLLHUP | EPOLLERR))
        return closeConnection(clientFd);
    client.setLastActivity();

    if (event & EPOLLIN) {
        if (recvClientEvent(client) == false)
            return closeConnection(clientFd);
    }
    if (event & EPOLLOUT) {
        if (client.sendResponse() == false)
            return closeConnection(clientFd);

        if (client.parsePending() == false)
            return closeConnection(clientFd);
    }
}

int Server::getIdentifier(void) const { return _sid; }
