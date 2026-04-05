#include "ServerManager.hpp"
#include "../CGI/CGI.hpp"
// # include "../Client/HttpRequest/HttpRequest.hpp"

#include <exception>
#include <iostream>
#include <vector>

#include <cerrno>
#include <csignal>
#include <cstdio>
#include <cstring>

#include <fcntl.h>
#include <netinet/in.h>
#include <poll.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>

ServerManager::ServerManager(const t_config config):
_config(config), requestBuf(config.maxClients) {
  for (size_t i = 0; i < this->config.maxClients; i++)
    this->_clients[i].setFd(-1);
}

ServerManager::~ServerManager(void) {
  int fd;
  for (size_t i = 3; i < this->config.maxClients; i++) {
    fd = this->_clients[i].getFd();
    if (fd != -1)
      close(fd);
  }
}

void ServerManager::setServerSockAddr(void) {
  this->serverSockAddr.sin_family = AF_INET;
  this->serverSockAddr.sin_port = htons(8080);
  this->serverSockAddr.sin_addr.s_addr = INADDR_ANY;
}

void ServerManager::addSocketToEpfd(int socketFd) {
  struct epoll_event ev;
  ev.events = EPOLLIN;
  ev.data.fd = socketFd;
  if (epoll_ctl(epfd, EPOLL_CTL_ADD, socketFd, &ev) == -1) {
    error_msg(ERR_EPOLL_CTL);
    throw std::exception();
  }
}

void ServerManager::initServerSocket(void) {
  this->serverSocket =
      socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0);
  if (this->serverSocket == -1) {
    error_msg(ERR_SOCKET);
    throw std::exception();
  }
  _clients[this->serverSocket].setFd(this->serverSocket);
  int opt = 1;
  if (setsockopt(this->serverSocket, SOL_SOCKET, SO_REUSEADDR, &opt,
                 sizeof(opt)) == -1) {
    error_msg(ERR_SETSOCKOPT);
    throw std::exception();
  }
}

void ServerManager::createEpoll(void) {
  this->epfd = epoll_create1(0);
  if (this->epfd == -1) {
    error_msg(ERR_EPOLL_CREATE1);
    throw std::exception();
  }
  _clients[this->epfd].setFd(this->epfd);
}

void ServerManager::bindAndListen(void) {
  if (bind(this->serverSocket, (struct sockaddr *)&this->serverSockAddr,
           sizeof(this->serverSockAddr)) == -1) {
    error_msg(ERR_BIND);
    throw std::exception();
  }
  if (listen(this->serverSocket, 5) == -1) {
    error_msg(ERR_LISTEN);
    throw std::exception();
  }
}

void	ServerManager::validateServerConfig(const t_server_config config) const {
	(void)config;
	if (false) { // TODO Implement
		throw std::exception();
	}
}

void	ServerManager::initServers() {
	// TODO Don't forget to limit maximum servers in config parser
	for (size_t i = 0; i < _config.serverConfigs.size(); i++) {
		try {
			validateServerConfig(_config.serverConfigs[i]);
		}
		catch (std::exception& e) {
			std::cerr << "ERROR: Invalid ServerConfig" << i << ": " << e.what() << std::endl;
			continue ;
		}
		Server	server(_config.serverConfigs[i]);
		_servers.push_back(server); // TODO nice rule of thumb: constructors should never fail or throw :D
	}
	for (size_t i = 0; i < _servers.size(); i++) {
		try {
			_servers.at(i).init();
		}
		catch (std::exception& e) {
			std::cerr << "ERROR: Couldn't init server: " << e.what() << std::endl;
			continue ;
		}
		std::cout << "Initialized Server " << i << ": " << _servers.at(i).getName() << " successfully" << std::endl;
	}
}

void	ServerManager::validateConfig() const { // TODO Implement
	if (false) { // TODO Implement
		throw std::exception();
	}
}

void ServerManager::init(void) { // TODO we can maybe put all this code in the constructor
	validateConfig();
	initServers();
  initServerSocket();
  setServerSockAddr();
  createEpoll();
  addSocketToEpfd(this->serverSocket);
  bindAndListen();
}

void ServerManager::epollWait() {
  this->readyEvents =
      epoll_wait(epfd, this->requestBuf.data(), this->config.maxClients, -1);
  // if (gSignalStatus)
  //   break;
  if (this->readyEvents == -1) {
    error_msg(ERR_EPOLL_WAIT);
    perror(strerror(errno));
    throw std::exception();
  }
}

void ServerManager::setToNonBlocking(int socketFd) {
  if (fcntl(socketFd, F_SETFL, FD_CLOEXEC | O_NONBLOCK) == -1) {
    error_msg(ERR_FCNTL);
    throw std::exception();
  }
}

void ServerManager::handleServerEvent(void) {
  int clientSocket;

  while (true) {
    clientSocket = accept(this->serverSocket, NULL, NULL);
    if (clientSocket == -1) {
      if (errno == EAGAIN || errno == EWOULDBLOCK) {
        return;
      } else {
        error_msg(ERR_ACCEPT);
        throw std::exception();
      }
      break;
    }
    _clients[clientSocket].setFd(clientSocket);
    setToNonBlocking(clientSocket);
    addSocketToEpfd(clientSocket);
    std::cout << "Client accepted: FD " << clientSocket << "\n";
  }
}
#define BUFFER_SIZE                                                            \
  4096 // we should move this to another spot but i need it to test parsing for
     // diff sizes
void ServerManager::handleClientEvent(const int clientFd) {
  char buffer[BUFFER_SIZE + 1];
  ssize_t bytesRead = 0;

  // std::cout << "message from client FD " << clientFd << " received!\n";
  bytesRead = recv(clientFd, buffer, BUFFER_SIZE, 0);
  if (bytesRead == 0) {
    std::cout << "client FD " << clientFd << " closed connection!\n";
    if (epoll_ctl(epfd, EPOLL_CTL_DEL, clientFd, NULL) == -1) {
      error_msg(ERR_EPOLL_CTL);
      throw std::exception();
    }
    if (close(clientFd) == -1) {
      error_msg(ERR_CLOSE);
      throw std::exception();
    }
    _clients[clientFd].reset();
    return;
  }
  if (bytesRead == -1) {
    error_msg(ERR_RECV);
    throw std::exception();
  }
  buffer[bytesRead] = 0;
  if (_clients[clientFd].loop(buffer) == 1) { // TODO passing char* to std::string parameter is implicitly converting. Be careful!
    std::cout << "client FD " << clientFd << " connection has been closed!\n";
    if (epoll_ctl(epfd, EPOLL_CTL_DEL, clientFd, NULL) == -1) {
      error_msg(ERR_EPOLL_CTL);
      throw std::exception();
    }
    if (close(clientFd) == -1) {
      error_msg(ERR_CLOSE);
      throw std::exception();
    }
    _clients[clientFd].reset();
    return;
  }
}

void ServerManager::handleCGI(void) const {
  CGI cgi;

  if (cgi.validateRequest(request)) {
    return;
  }
  cgi.initCGI(request);
  cgi.pipeIO();
  cgi.spawnProcess();
  cgi.wait();
  // cgi.redirectIO(); // I don't think I need this ¯\_(ツ)_/¯
}

void ServerManager::loopReadyEvents(
    void) { // this loop is only meant for 1 server, epoll doesnt know what fd
            // belongs to which server
  for (int i = 0; i < this->readyEvents; i++) {
    int fd = requestBuf[i].data.fd;
    if (fd == serverSocket) {
      handleServerEvent();
    } else {
      handleClientEvent(fd);
      // parseHttpRequest();
      // this->request.print();
      // handleCGI();
    }
  }
}
