#include "Server.hpp"

#include <iostream>

#include <cerrno>

#include <fcntl.h>
#include <unistd.h>

Server::Server(const t_server_config& config, int epfd, std::map<int, IntSet>& clientsMap):
_config(config), _name(config.name), _epfd(epfd), _clientsMap(clientsMap) {
	for (size_t i = 0; i < _config.maxClients; i++) {
		_clients[i].setFd(-1);
	}
}

Server::Server(const Server& obj):
_config(obj._config), _name(obj._name), _serverSocket(obj._serverSocket),
_serverSockAddr(obj._serverSockAddr), _epfd(obj._epfd), _clientsMap(obj._clientsMap) {
	for (size_t i = 0; i < _config.maxClients; i++) {
		_clients[i] = obj._clients[i];
	}
}

Server::~Server(void) {}

void	Server::closeClientFds(void) const {
	int fd;

	for (size_t i = 3; i < _config.maxClients; i++) {
		fd = _clients[i].getFd();
		if (fd != -1)
			close(fd);
	}
}

void	Server::removeClientFd(const int clientFd) {
	_clientsMap.at(_serverSocket).erase(clientFd);
}

void	Server::closeConnection(int clientFd) {
	removeClientFd(clientFd);
	std::cout << "Server __" << _name << "__ closed connection with Client " << clientFd << std::endl;
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

void Server::setToNonBlocking(int socketFd) {
  if (fcntl(socketFd, F_SETFL, FD_CLOEXEC | O_NONBLOCK) == -1) {
	error_msg(ERR_FCNTL);
	throw std::exception();
  }
}

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
	_serverSockAddr.sin_port = htons(_config.host);
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
	if (listen(_serverSocket, 5) == -1) { // TODO hardocded 5?
		error_msg(ERR_LISTEN);
		throw std::exception();
	}
}

int		Server::start(void) {
	initServerSocket();
	setServerSockAddr();
	addSocketToEpoll(_serverSocket);
	bindAndListen();
	return _serverSocket;
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
		}
		_clients[clientSocket].setFd(clientSocket);
		_clientsMap.at(_serverSocket).insert(clientSocket);
		setToNonBlocking(clientSocket);
		addSocketToEpoll(clientSocket);
		std::cout << "Server __" << _name << "__ accepted Client: " << clientSocket << std::endl;
	}
}

#define BUFFER_SIZE                                                            \
  4096 // we should move this to another spot but i need it to test parsing for
	 // diff sizes
void	Server::handleClientEvent(const int clientFd) {
	char	buffer[BUFFER_SIZE + 1];
	ssize_t	bytesRead = 0;

	// std::cout << "message from client FD " << clientFd << " received!\n";
	bytesRead = recv(clientFd, buffer, BUFFER_SIZE, 0);
	if (bytesRead == 0) {
		closeConnection(clientFd);
		return ;
	}
	if (bytesRead == -1) {
		error_msg(ERR_RECV);
		closeConnection(clientFd);
		return ;
	}
	buffer[bytesRead] = 0;
	if (_clients[clientFd].loop(buffer) == 1) {
		closeConnection(clientFd);
		return ;
	}
}

std::string	Server::getName(void) const {
	return _name;
}
