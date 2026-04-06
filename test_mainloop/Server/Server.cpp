#include "Server.hpp"

#include <iostream>

#include <fcntl.h>
#include <unistd.h>

#define CLIENTS 1024 // FIXME hardcode

Server::Server(const t_server_config& config, int epfd):
_config(config), _name(config.name), _epfd(epfd) {
	for (size_t i = 0; i < CLIENTS; i++) {
		_clients[i].setFd(-1);
	}
}

Server::~Server(void) {
	int fd;

	for (size_t i = 3; i < CLIENTS; i++) {
		fd = _clients[i].getFd();
		if (fd != -1)
			close(fd);
	}
}

// Server::Server(const Server& obj): // TODO Implement
// config(obj.config) {}

// Server&	Server::operator=(const Server& obj) {
// 	if (this == &obj) {
// 		return *this;
// 	}
// 	// TODO Implement
// 	return *this;
// }

void	Server::initServerSocket(void) {
	_serverSocket = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0);
	if (_serverSocket == -1) {
		error_msg(ERR_SOCKET);
		throw std::exception();
	}
	_clients[_serverSocket].setFd(_serverSocket); // TODO should we also add it to ServerManager's _clients?
	int opt = 1;
	if (setsockopt(_serverSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1) {
		error_msg(ERR_SETSOCKOPT);
		throw std::exception();
	}
}

void	Server::setServerSockAddr(void) {
	_serverSockAddr.sin_family = AF_INET;
	_serverSockAddr.sin_port = htons(8080); // FIXME from conf
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
	if (listen(_serverSocket, 5) == -1) {
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

void	Server::handleServerEvent(std::map<int, IntSet>& ManagersClients) {
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
		ManagersClients.at(_serverSocket).insert(clientSocket);
		setToNonBlocking(clientSocket);
		addSocketToEpoll(clientSocket);
		std::cout << "Server __" << _name << "__: Client accepted: FD " << clientSocket << "\n";
	}
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

std::string	Server::getName(void) const {
	return _name;
}
