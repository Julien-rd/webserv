#include "Server.hpp"

#include <iostream>

#include <cerrno>

#include <fcntl.h>
#include <unistd.h>

#define CLIENTS 1024 // FIXME hardcode

// Server::Server(int epfd):
// _epfd(epfd) {
// 	for (size_t i = 0; i < CLIENTS; i++) {
// 		_clients[i].setFd(-1);
// 	}
// }

// Server::Server(void) {
// 	for (size_t i = 0; i < CLIENTS; i++) {
// 		_clients[i].setFd(-1);
// 	}
// }

Server::Server(const t_server_config& config, int epfd, std::map<int, IntSet>& clientsMap):
_config(config), _name(config.name), _epfd(epfd), _clientsMap(clientsMap) {
	for (size_t i = 0; i < CLIENTS; i++) {
		_clients[i].setFd(-1);
	}
}

Server::Server(const Server& obj):
_config(obj._config), _name(obj._name), _serverSocket(obj._serverSocket),
_serverSockAddr(obj._serverSockAddr), _epfd(obj._epfd), _clientsMap(obj._clientsMap) {
	for (size_t i = 0; i < CLIENTS; i++) {
		_clients[i] = obj._clients[i];
	}
}

Server::~Server(void) {}

// Server&	Server::operator=(const Server& obj) {
// 	if (this == &obj) {
// 		return *this;
// 	}
// 	_config = obj._config;
// 	_epfd = obj._epfd;
// 	for (size_t i = 0; i < CLIENTS; i++) {
// 		_clients[i] = obj._clients[i];
// 	}
// 	return *this;
// }

void	Server::initServerSocket(void) {
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

void	Server::setServerSockAddr(void) {
	_serverSockAddr.sin_family = AF_INET;
	_serverSockAddr.sin_port = htons(_config.host); // FIXME from conf
	_serverSockAddr.sin_addr.s_addr = INADDR_ANY;
}

void	Server::addSocketToEpoll(int socketFd) {
	struct epoll_event	ev;
	ev.events = EPOLLIN;
	ev.data.fd = socketFd;
	if (epoll_ctl(_epfd, EPOLL_CTL_ADD, socketFd, &ev) == -1) {
		error_msg(ERR_EPOLL_CTL);
		throw std::exception();
	}
}

void	Server::bindAndListen(void) {
	if (bind(_serverSocket, (struct sockaddr *)&_serverSockAddr, sizeof(_serverSockAddr)) == -1) {
		error_msg(ERR_BIND);
		throw std::exception();
	}
	if (listen(_serverSocket, 5) == -1) { // TODO hardocded 5 ?
		error_msg(ERR_LISTEN);
		throw std::exception();
	}
}

void Server::setToNonBlocking(int socketFd) {
  if (fcntl(socketFd, F_SETFL, FD_CLOEXEC | O_NONBLOCK) == -1) {
	error_msg(ERR_FCNTL);
	throw std::exception();
  }
}

void	Server::handleServerEvent(void) {
	int clientSocket;

	while (true) {
		clientSocket = accept(_serverSocket, NULL, NULL);
		if (clientSocket == -1) {
			if (errno == EAGAIN || errno == EWOULDBLOCK) {
				return;
			}
			else {
				error_msg(ERR_ACCEPT);
				throw std::exception();
			}
			break;
		} // TODO FIXXXXXXXXXXXXXxFIXXXXXXXXXXXXXxFIXXXXXXXXXXXXXxFIXXXXXXXXXXXXXxFIXXXXXXXXXXXXXxFIXXXXXXXXXXXXXxFIXXXXXXXXXXXXXxFIXXXXXXXXXXXXXxFIXXXXXXXXXXXXXxFIXXXXXXXXXXXXXx means 1024 limit is shared between all servers
		_clients[clientSocket].setFd(clientSocket); // FIXME !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! Indexing with the new clientSocket FD is now shared between all servers !!!!!!!!!! MUST FIX FIRST
		_clientsMap.at(_serverSocket).insert(clientSocket);
		setToNonBlocking(clientSocket);
		addSocketToEpoll(clientSocket);
		std::cout << "Server __" << _name << "__: Client accepted: FD " << clientSocket << "\n";
	}
}

void	Server::removeClientFd(const int clientFd) {
	_clientsMap.at(_serverSocket).erase(clientFd);
}

#define BUFFER_SIZE                                                            \
  4096 // we should move this to another spot but i need it to test parsing for
	 // diff sizes
void	Server::handleClientEvent(const int clientFd) {
  char buffer[BUFFER_SIZE + 1];
  ssize_t bytesRead = 0;

  // std::cout << "message from client FD " << clientFd << " received!\n";
  bytesRead = recv(clientFd, buffer, BUFFER_SIZE, 0);
  if (bytesRead == 0) {
	removeClientFd(clientFd);
	std::cout << "Server __" << _name << "__: client FD " << clientFd << " closed connection!\n";
	if (epoll_ctl(_epfd, EPOLL_CTL_DEL, clientFd, NULL) == -1) {
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
	removeClientFd(clientFd);
	std::cout << "Server __" << _name << "__: client FD " << clientFd << " connection has been closed!\n";
	if (epoll_ctl(_epfd, EPOLL_CTL_DEL, clientFd, NULL) == -1) {
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

int		Server::start(void) {
	initServerSocket();
	setServerSockAddr();
	addSocketToEpoll(_serverSocket);
	bindAndListen();
	return _serverSocket;
}

void	Server::closeClientFds(void) const {
	int fd;

	for (size_t i = 3; i < CLIENTS; i++) {
		fd = _clients[i].getFd();
		if (fd != -1)
			close(fd);
	}
}

std::string	Server::getName(void) const {
	return _name;
}

// int			Server::getServerSocket(void) const {
// 	return _serverSocket;
// }

// void		Server::setConfig(const t_server_config& config) {
// 	_config = config;
// }

// // void		Server::setEpfd(int epfd) {
// // 	_epfd = epfd;
// // }
