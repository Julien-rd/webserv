#include "Server.hpp"

Server::Server(const t_server_config& config):
_config(config) {}

Server::~Server(void) {}

// Server::Server(const Server& obj): // TODO Implement
// config(obj.config) {}

// Server&	Server::operator=(const Server& obj) {
// 	if (this == &obj) {
// 		return *this;
// 	}
// 	// TODO Implement
// 	return *this;
// }

void Server::initServerSocket(void) {
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

void Server::setServerSockAddr(void) {
  _serverSockAddr.sin_family = AF_INET;
  _serverSockAddr.sin_port = htons(8080); // FIXME from conf
  _serverSockAddr.sin_addr.s_addr = INADDR_ANY;
}

void Server::addSocketToEpoll(int socketFd) {
  struct epoll_event ev;
  ev.events = EPOLLIN;
  ev.data.fd = socketFd;
  if (epoll_ctl(epfd, EPOLL_CTL_ADD, socketFd, &ev) == -1) {
	error_msg(ERR_EPOLL_CTL);
	throw std::exception();
  }
}

void Server::createEpoll(void) {
  this->epfd = epoll_create1(0);
  if (this->epfd == -1) {
	error_msg(ERR_EPOLL_CREATE1);
	throw std::exception();
  }
  _clients[this->epfd].setFd(this->epfd);
}

void Server::bindAndListen(void) {
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

void	Server::init(void) {
	_name = _config.name;
	initServerSocket();
	setServerSockAddr();
	createEpoll(); // TODO this should be in server manager. no?
	addSocketToEpfd(this->serverSocket);
	bindAndListen();
}

std::string	Server::getName(void) const {
	return _name;
}
