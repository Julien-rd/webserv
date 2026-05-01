#include "Server.hpp"

#include <iostream>

#include <cerrno>

#include <arpa/inet.h>
#include <errno.h>
#include <error.h>
#include <fcntl.h>
#include <netdb.h>
#include <stdexcept>
#include <string>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

Server::Server(const t_server &config, std::map<int, IntSet> &clientsMap,
               std::map<int, Client> &clients,
               std::map<int, int> &clientToServerMap, t_serverContext &context,
               int sid)
    : _config(config), _sid(sid), _context(context), _clientsMap(clientsMap),
      _clientToServerMap(clientToServerMap), _clients(clients) {}

Server::Server(const Server &obj)
    : _config(obj._config), _sid(obj._sid), _context(obj._context),
      _serverSocket(obj._serverSocket), _addrInfo(obj._addrInfo),
      _clientsMap(obj._clientsMap), _clientToServerMap(obj._clientToServerMap),
      _clients(obj._clients) {}

Server::~Server(void) {}

void Server::closeClientFds(void) {
  int fd;

  for (std::map<int, Client>::iterator it = _clients.begin();
       it != _clients.end(); it++) {
    fd = it->second.getFd();
    if (fd != -1)
      close(fd);
  }
}

void Server::updateClientsMap(enum e_map_operation op, const int clientFd) {
  switch (op) {
  case ADD:
    _clientToServerMap[clientFd] = _serverSocket;
    _clients[clientFd] = Client(
        _context.epfd); // TODO can we construct an entry in the map in a better
                        // way than constructing and then calling copy
                        // assignment operator? this basically constructs 2
                        // client instances, can we make it only one?
    _clients[clientFd].setFd(clientFd);
    _clientsMap.at(_serverSocket).insert(clientFd);
    break;
  case REMOVE:
    _clientToServerMap.erase(clientFd);
    _clients[clientFd].reset();
    _clients.erase(clientFd);
    _clientsMap.at(_serverSocket).erase(clientFd);
  }
}

void Server::closeConnection(int clientFd) {
  updateClientsMap(REMOVE, clientFd);
  std::cout << "Server __" << _sid << "__ closed connection with Client "
            << clientFd << std::endl;
  if (epoll_ctl(_context.epfd, EPOLL_CTL_DEL, clientFd, NULL) == -1) {
    error_msg(ERR_EPOLL_CTL);
    throw std::runtime_error("couldn't close connection");
  }
  if (close(clientFd) == -1) {
    error_msg(ERR_CLOSE);
    throw std::runtime_error("couldn't close connection");
  }
  return;
}

void Server::setToNonBlocking(int socketFd) {
  if (fcntl(socketFd, F_SETFL, FD_CLOEXEC | O_NONBLOCK) == -1) {
    error_msg(ERR_FCNTL);
    throw std::runtime_error("couldn't set fd to nonblocking");
  }
}

void Server::initServerSocket(void) {
  _serverSocket =
      socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0);
  if (_serverSocket == -1) {
    error_msg(ERR_SOCKET);
    throw std::runtime_error("couldn't init server socket");
  }
  int opt = 1;
  if (setsockopt(_serverSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) ==
      -1) {
    error_msg(ERR_SETSOCKOPT);
    throw std::runtime_error("couldn't init server socket");
  }
}

void Server::setServerSockAddr(void) {
  addrinfo hints = {0, 0, 0, 0, 0, 0, 0, 0};
  int res;

  hints.ai_socktype = SOCK_STREAM;
  hints.ai_family = AF_UNSPEC;
  hints.ai_flags = AI_NUMERICHOST;
  std::cout << "ip: " << _config.ip.c_str() << " == ";
  res =
      getaddrinfo(_config.ip.c_str(), _config.port.c_str(), &hints, &_addrInfo);
  if (res) {
    throw std::runtime_error(std::string("gettaddrinfo() failed: ") +
                             gai_strerror(res));
  }
}

void Server::addSocketToEpoll(int socketFd) {
  struct epoll_event ev;
  ev.events = EPOLLIN;
  // ev.data.ptr = 0;
  ev.data.fd = socketFd;
  if (epoll_ctl(_context.epfd, EPOLL_CTL_ADD, socketFd, &ev) == -1) {
    error_msg(ERR_EPOLL_CTL);
    throw std::runtime_error("couldn't add socket to epoll");
  }
}

void Server::bindAndListen(void) {
  if (bind(_serverSocket, _addrInfo->ai_addr, _addrInfo->ai_addrlen) == -1) {
    freeaddrinfo(_addrInfo);
    error_msg(ERR_BIND);
    throw std::runtime_error("couldn't bind server");
  }
  freeaddrinfo(_addrInfo);
  if (listen(_serverSocket, 20) == -1) { // TODO hardocded 20?
    error_msg(ERR_LISTEN);
    throw std::runtime_error("couldn't listen from server");
  }
}

int Server::start(void) {
  initServerSocket();
  setServerSockAddr();
  addSocketToEpoll(_serverSocket);
  bindAndListen();
  return _serverSocket;
}

void Server::checkClientCap(void) {
  if (_clients.size() == _context.maxClients) { // FIX
    throw std::runtime_error(
        "WARNING: client capacity reached. can't accept more connections");
  }
}

void Server::handleServerEvent(void) {
  int clientSocket;

  while (true) {
    clientSocket = accept(_serverSocket, NULL, NULL);
    if (clientSocket == -1) {
      if (errno == EAGAIN || errno == EWOULDBLOCK) {
        return;
      } else {
        error_msg(ERR_ACCEPT);
        throw std::exception();
      }
    }
    try {
      checkClientCap();
    } catch (std::exception &e) {
      std::cout << e.what() << std::endl;
      return;
    }
    updateClientsMap(ADD, clientSocket);
    setToNonBlocking(clientSocket);
    addSocketToEpoll(clientSocket);
    std::cout << "Server __" << _sid << "__ accepted Client: " << clientSocket
              << std::endl;
  }
}

void Server::handleClientEvent(const int clientFd) {
  char buffer[BUFFER_SIZE + 1];
  ssize_t bytesRead = 0;

  bytesRead = recv(clientFd, buffer, BUFFER_SIZE, 0);
  if (bytesRead == 0) {
    closeConnection(clientFd);
    return;
  }
  if (bytesRead == -1) {
    error_msg(ERR_RECV);
    closeConnection(clientFd);
    return;
  }
  buffer[bytesRead] = 0;
  if (_clients[clientFd].loop(buffer) == 1) {
    closeConnection(clientFd);
    return;
  }
}

int Server::getIdentifier(void) const { return _sid; }
