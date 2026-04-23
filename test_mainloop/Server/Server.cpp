#include "Server.hpp"

#include <iostream>
#include <sstream>

#include <cerrno>

#include <fcntl.h>
#include <stdexcept>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string>
#include <error.h>
#include <errno.h>

Server::Server(const t_server& config, int epfd,
	std::map<int, IntSet>& clientsMap, std::map<int, Client>& clients, std::map<int, int>& clientToServerMap, int sid):
_config(config), _sid(sid), _epfd(epfd), _clientsMap(clientsMap), _clientToServerMap(clientToServerMap), _clients(clients) {}

Server::Server(const Server& obj):
_config(obj._config), _sid(obj._sid),
_serverSocket(obj._serverSocket), _addrInfo(obj._addrInfo), _epfd(obj._epfd),
_clientsMap(obj._clientsMap), _clientToServerMap(obj._clientToServerMap), _clients(obj._clients) {}

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
			_clientToServerMap[clientFd] = _serverSocket;
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
	addrinfo	hints = {0, 0, 0, 0, 0, 0, 0, 0};
	int			res;

	hints.ai_socktype = SOCK_STREAM;
	hints.ai_family = AF_UNSPEC;
	hints.ai_flags = AI_NUMERICHOST;
	std::stringstream iss;
	iss << _config.port;
	std::cout << "ip is: " << _config.ip.c_str() << "\n";
	res = getaddrinfo(_config.ip.c_str(), iss.str().c_str(), &hints, &_addrInfo);
	if (res) {
		std::cout << "failed with err: " << gai_strerror(res) << std::endl;
		throw std::runtime_error("gettaddrinfo() failed");
	}
	// _serverSockAddr.sin_family = AF_INET;
	// _serverSockAddr.sin_port = htons(_config.port);
	// std::cout << "in setSErverSockAddr: _config.ip: " << _config.ip << " == inet_addr(_config.ip): " << inet_addr(_config.ip.c_str()) << std::endl;
	// _serverSockAddr.sin_addr.s_addr = inet_addr(_config.ip.c_str()); // FIX IT: this needs to be modifiable with config ip
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
	// std::cout << "server _" << _serverSocket << "_ _addrInfo: " << _addrInfo->ai_addr << " == " << std::endl;
	// std::cout << _serverSocket;
	// std::cout << _addrInfo->ai_addr;
	// std::cout << _addrInfo->ai_addrlen;
	// std::cout << std::endl;
	if (bind(_serverSocket, _addrInfo->ai_addr, _addrInfo->ai_addrlen) == -1) {
		// std::cerr << "errno is: " << hstrerror(errno) << std::endl;
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
	if (_clients.size() == 1024 /* _config.maxClients */) { // FIX: either every server stores max client or we create a context struct with the maps, globals, and epfd (maxclients is a global)
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
	if (_clients[clientFd].loop(buffer) == 1) {
		closeConnection(clientFd);
		return ;
	}
}

int	Server::getIdentifier(void) const {
	return _sid;
}
