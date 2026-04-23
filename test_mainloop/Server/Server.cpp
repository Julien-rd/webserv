#include "Server.hpp"

#include <iostream>

#include <cerrno>

#include <fcntl.h>
#include <unistd.h>
#include <arpa/inet.h>

Server::Server(const t_server& config, const t_serverContext& context, std::map<int, IntSet>& clientsMap, std::map<int, Client>& clients, int sid):
_config(config), _context(context), _sid(sid),  _clientsMap(clientsMap), _clients(clients) {}

Server::Server(const Server& obj):
_config(obj._config),  _context(obj._context),_sid(obj._sid),
_serverSocket(obj._serverSocket), _serverSockAddr(obj._serverSockAddr),
 _clientsMap(obj._clientsMap), _clients(obj._clients) {}

Server::~Server(void) {}

void	Server::closeClientFds(void) {
	int fd;

	for (std::map<int, Client>::iterator it = _clients.begin(); it != _clients.end(); it++) {
		fd = it->second.getFd();
		if (fd != -1)
			close(fd);
	}
}

void	Server::updateClientsMap(enum e_operation operation, const int clientFd) {
	switch (operation) {
		case ADD:
			_clients[clientFd].setFd(clientFd); // This constructs a client instance at this key implicitly
			_clientsMap.at(_serverSocket).insert(clientFd);
			break;
		case REMOVE:
			_clients[clientFd].reset();
			_clients.erase(clientFd);
			_clientsMap.at(_serverSocket).erase(clientFd);
	}
}

void	Server::closeConnection(int clientFd) {
	updateClientsMap(REMOVE, clientFd);
	std::cout << "Server __" << _sid << "__ closed connection with Client " << clientFd << std::endl;
	if (epoll_ctl(_epfd, EPOLL_CTL_DEL, clientFd, NULL) == -1) {
	  error_msg(ERR_EPOLL_CTL);
	  throw std::exception();
	}
	if (close(clientFd) == -1) {
	  error_msg(ERR_CLOSE);
	  throw std::exception();
	}
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
	_serverSockAddr.sin_port = htons(_config.port);
	_serverSockAddr.sin_addr.s_addr = inet_addr(_config.ip.c_str()); // TODO: this needs to be modifiable with config ip
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

void	Server::checkClientCap(void) {
	if (_clients.size() ==  _context.maxClients) {
		throw std::runtime_error("WARNING: client capacity reached. can't accept more connections");
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
		}
		try {
			checkClientCap();
		}
		catch (std::exception& e) {
			std::cout << e.what() << std::endl;
			return ;
		}
		updateClientsMap(ADD, clientSocket);
		setToNonBlocking(clientSocket);
		addSocketToEpoll(clientSocket);
		std::cout << "Server __" << _sid << "__ accepted Client: " << clientSocket << std::endl;
	}
}

void	Server::handleClientEvent(const int clientFd) {
	char	buffer[BUFFER_SIZE + 1];
	ssize_t	bytesRead = 0;

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
	std::cout << "hifrom loop\n";
	if (_clients[clientFd].loop(buffer) == 1) {
		closeConnection(clientFd);
		return ;
	}
}

int	Server::getIdentifier(void) const {
	return _sid;
}
