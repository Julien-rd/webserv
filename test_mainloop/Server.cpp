#include "Server.hpp"

#include <exception>
#include <iostream>
#include <vector>

#include <cerrno>
#include <csignal>
#include <cstring>
#include <cstdio>

#include <unistd.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <poll.h>
#include <sys/epoll.h>
#include <sys/socket.h>

Server::Server(const int maxClients):
maxClients(maxClients),
openFds(new int[maxClients]),
requestBuf(maxClients) {
	for (size_t i = 3; i < this->maxClients; i++) {
		this->openFds[i] = -1;
	}
}

Server::~Server(void) {
	size_t	iter = 3;
	for (size_t i = 3; i < this->maxClients; i++) {
		if (this->openFds[iter] != -1) {
			close(this->openFds[iter]);
		}
	}
	delete[] this->openFds;
}

void	Server::setServerSockAddr(void) {
  this->serverSockAddr.sin_family = AF_INET;
  this->serverSockAddr.sin_port = htons(8080);
  this->serverSockAddr.sin_addr.s_addr = INADDR_ANY;
}

void	Server::addSocketToEpfd(int socketFd) {
	struct epoll_event	ev;
	ev.events = EPOLLIN | EPOLLET;
	ev.data.fd = socketFd;
	if (epoll_ctl(epfd, EPOLL_CTL_ADD, socketFd, &ev) == -1) {
		error_msg(ERR_EPOLL_CTL);
		throw std::exception();
	}
}

void	Server::initServerSocket(void) {
	this->serverSocket = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
	if (this->serverSocket == -1) {
		error_msg(ERR_SOCKET);
		throw std::exception();
	}
	this->openFds[this->serverSocket] = this->serverSocket;
	int opt = 1;
	if (setsockopt(this->serverSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1) {
		error_msg(ERR_SETSOCKOPT);
		throw std::exception();
	}
}

void	Server::createEpoll(void) {
	this->epfd = epoll_create1(0);
	if (this->epfd == -1) {
		error_msg(ERR_EPOLL_CREATE1);
		throw std::exception();
	}
	this->openFds[this->epfd] = this->epfd;
}

void	Server::bindAndListen(void) {
	if (bind(this->serverSocket, (struct sockaddr *)&this->serverSockAddr, sizeof(this->serverSockAddr)) == -1) {
		error_msg(ERR_BIND);
		throw std::exception();
	}
	if (listen(this->serverSocket, 5) == -1) {
		error_msg(ERR_LISTEN);
		throw std::exception();
	}
}

void	Server::initServer(void) { // TODO we can maybe put all this code in the constructor
	initServerSocket();
	setServerSockAddr();
	createEpoll();
	addSocketToEpfd(this->serverSocket);
	bindAndListen();
}

void	Server::epollWait() {
    this->readyEvents = epoll_wait(epfd, this->requestBuf.data(), this->maxClients, -1);
    // if (gSignalStatus)
    //   break;
    if (this->readyEvents == -1) {
		error_msg(ERR_EPOLL_WAIT);
		perror(strerror(errno));
		throw std::exception();
	}
}

void	Server::setToNonBlocking(int socketFd) {
	int flags = fcntl(socketFd, F_GETFL, 0); // FIXME We're only allowed F_SETFL, O_NONBLOCK and FD_CLOEXEC with fcntl()
	if (flags == -1) {
		error_msg(ERR_FCNTL);
		throw std::exception();
	}
	flags = flags | O_NONBLOCK;
	if (fcntl(socketFd, F_SETFL, flags) == -1) {
		error_msg(ERR_FCNTL);
		throw std::exception();
	}
}

void	Server::handleServerEvent(void) {
	int	ClientSocket;
	while (true) {
		ClientSocket = accept(this->serverSocket, NULL, NULL);
		if (ClientSocket == -1) {
			if (errno == EAGAIN || errno == EWOULDBLOCK) {
				return ;
			}
			else {
				error_msg(ERR_ACCEPT);
				throw std::exception();
			}
			break;
		}
		this->openFds[ClientSocket] = ClientSocket;
		setToNonBlocking(ClientSocket);
		addSocketToEpfd(ClientSocket);
		std::cout << "Client accepted: FD " << ClientSocket << "\n";
	}
}

void	Server::handleClientEvent(const int clientFd) {
	char	buffer[10];
	ssize_t	bytesRead;

	std::cout << "message from client FD " << clientFd << " received!\n";
	while (1) {
		bytesRead = read(clientFd, buffer, 10);
		if (bytesRead == 0) {
			std::cout << "client FD " << clientFd << " closed connection!\n";
			if (epoll_ctl(epfd, EPOLL_CTL_DEL, clientFd, NULL) == -1) {
				error_msg(ERR_EPOLL_CTL);
				throw std::exception();
			}
			this->openFds[clientFd] = -1;
			if (close(clientFd) == -1) {
				error_msg(ERR_READ);
				throw std::exception();
			}
			break;
		}
		if (bytesRead == -1) {
			if (error_msg(ERR_READ) == 1)
				throw std::exception();
			else {
				break;
			}
		}
		buffer[bytesRead] = 0;
		std::cout << buffer;
	}
}

void	Server::loopReadyEvents(void) {
	for (int i = 0; i < this->readyEvents; i++) {
		int fd = requestBuf[i].data.fd;
		if (fd == serverSocket) {
			handleServerEvent();
		} else {
			handleClientEvent(fd);
		}
	}
}

int	main(void) {
	t_configParser	parser;
	parser.maxClients = 1024;
	Server			server(parser.maxClients);

	// try { // TODO Remove this try catch, because failing to initialize the server should exit, right? :D (basically exceptions should be used for things that could fail but prog still continues execution after). Also if u keep it, we shouldn't debug using exceptions prints
	server.initServer();
	// }
	// catch (std::exception& e) {
	// 	std::cerr << e.what() << std::endl;
	// 	return 1;
	// }

  while (1) {
    std::cout << "waiting for request \n";
	try {
		server.epollWait();
		server.loopReadyEvents();
	}
	catch (std::exception& e) {
		std::cerr << e.what() << std::endl;
		return 1;
	}
  }
}
