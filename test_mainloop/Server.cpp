#include "Server.hpp"

#include <exception>
#include <iostream>
#include <vector>

#include <cerrno>
#include <csignal>
#include <cstring>

#include <unistd.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <poll.h>
#include <sys/epoll.h>
#include <sys/socket.h>

Server::Server(const int maxClients):
maxClients(maxClients),
openFds(new int[maxClients]) {}

Server::~Server(void) {
	size_t iter = 3;
	while (iter < this->maxClients) {
		if (this->openFds[iter] != -1) {
			close(this->openFds[iter]);
		}
		++iter;
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

void	Server::initSocket(void) {
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
	initSocket();
	setServerSockAddr();
	createEpoll();
	addSocketToEpfd(this->serverSocket);
	bindAndListen();
}

typedef struct s_configParser { // TODO This just contains things we get from the config parser.
	size_t	maxClients;
}	t_configParser;

int	main(void) {
	t_configParser	parser;
	parser.maxClients = 1024;
	Server			server(parser.maxClients);

	try { // TODO Remove this try catch, because failing to initialize the server should exit, right? :D (basically exceptions should be used for things that could fail but prog still continues execution after). Also if u keep it, we shouldn't debug using exceptions prints
		server.initServer();
	}
	catch (std::exception& e) {
		std::cerr << e.what() << std::endl;
		return 1;
	}

  std::vector<epoll_event> request_buf(parser.maxClients);
  // TODO remake this :)
  while (1) {
    std::cout << "waiting for request \n";
    int ready_events = epoll_wait(epfd, request_buf.data(), parser.maxClients, -1);
    if (gSignalStatus)
      break;
    if (ready_events == -1)
      return error_msg(ErrorFlag::ERR_EPOLL_WAIT, open_fds);
    for (int i = 0; i < ready_events; i++) {
      int fd = request_buf[i].data.fd;
      if (fd == serverSocket) {
        while (true) {
          int ClientSocket = accept(serverSocket, NULL, NULL); // TODO can't do like socket(), accept4() not allowed and is a nonstandard Linux extension
          if (ClientSocket == -1) {
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
      } else {
        std::cout << "message from client FD " << fd << " received!\n";
        char buffer[10];
        while (1) {
          ssize_t bytes_read = read(fd, buffer, 10);
          if (bytes_read == 0) {
            std::cout << "client FD " << fd << " closed connection!\n";
            if (epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL) == -1)
              return error_msg(ErrorFlag::ERR_EPOLL_CTL, open_fds);
            open_fds[fd] = -1;
            if (close(fd) == -1)
              return error_msg(ErrorFlag::ERR_READ, open_fds);
            break;
          }
          if (bytes_read == -1) {
            if (error_msg(ErrorFlag::ERR_READ, open_fds) == 1)
              return 1;
            else {
              break;
            }
          }
          buffer[bytes_read] = 0;
          std::cout << buffer;
        }
      }
    }
  }
}
