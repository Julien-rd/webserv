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
_config(config), _requestBuf(CLIENTS) {} // FIXME hardcoded here

ServerManager::~ServerManager(void) {}

void	ServerManager::validateServerConfig(const t_server_config config) const {
	(void)config;
	if (false) { // TODO Implement
		throw std::exception();
	}
}

void	ServerManager::validateConfig() const { // TODO Implement
	if (false) { // TODO Implement
		throw std::exception();
	}
	// TODO Don't forget to limit maximum servers in config parser
}

void	ServerManager::createEpoll(void) {
	_epfd = epoll_create1(0);
	if (_epfd == -1) {
		error_msg(ERR_EPOLL_CREATE1);
		throw std::exception();
	}
	// _clients[_epfd].setFd(_epfd);
}

void	ServerManager::startServers() {
	int	serverSocket;

	for (size_t i = 0; i < _config.serverConfigs.size(); i++) {
		try {
			validateServerConfig(_config.serverConfigs[i]);
			serverSocket = _servers.at(i).start();
		}
		catch (std::exception& e) {
			std::cerr << "ERROR: Couldn't start server: " << e.what() << std::endl;
			continue ;
		}
		std::cout << "Started Server " << i << ": " << _servers.at(i).getName() << " successfully" << std::endl;
		Server	server(_config.serverConfigs[i], _epfd);
		_servers.insert(std::pair<int, Server>(serverSocket, server));
		// _serversFds.insert(serverSocket);
		
	}
}

void	ServerManager::init(void) {
	validateConfig();
	createEpoll();
	// initServers();
	startServers();
}

void	ServerManager::epollWait() {
	_readyEvents = epoll_wait(_epfd, _requestBuf.data(), CLIENTS, -1); // FIXME hardcode
	// if (gSignalStatus)
	//   break;
	if (_readyEvents == -1) {
		error_msg(ERR_EPOLL_WAIT);
		perror(strerror(errno));
		throw std::exception();
	}
}

// void ServerManager::handleServerEvent(void) {
// }

// #define BUFFER_SIZE                                                            
//   4096 // we should move this to another spot but i need it to test parsing for
// 	 // diff sizes
// void ServerManager::handleClientEvent(const int clientFd) {
//   char buffer[BUFFER_SIZE + 1];
//   ssize_t bytesRead = 0;

//   // std::cout << "message from client FD " << clientFd << " received!\n";
//   bytesRead = recv(clientFd, buffer, BUFFER_SIZE, 0);
//   if (bytesRead == 0) {
// 	std::cout << "client FD " << clientFd << " closed connection!\n";
// 	if (epoll_ctl(epfd, EPOLL_CTL_DEL, clientFd, NULL) == -1) {
// 	  error_msg(ERR_EPOLL_CTL);
// 	  throw std::exception();
// 	}
// 	if (close(clientFd) == -1) {
// 	  error_msg(ERR_CLOSE);
// 	  throw std::exception();
// 	}
// 	_clients[clientFd].reset();
// 	return;
//   }
//   if (bytesRead == -1) {
// 	error_msg(ERR_RECV);
// 	throw std::exception();
//   }
//   buffer[bytesRead] = 0;
//   if (_clients[clientFd].loop(buffer) == 1) { // TODO passing char* to std::string parameter is implicitly converting. Be careful!
// 	std::cout << "client FD " << clientFd << " connection has been closed!\n";
// 	if (epoll_ctl(epfd, EPOLL_CTL_DEL, clientFd, NULL) == -1) {
// 	  error_msg(ERR_EPOLL_CTL);
// 	  throw std::exception();
// 	}
// 	if (close(clientFd) == -1) {
// 	  error_msg(ERR_CLOSE);
// 	  throw std::exception();
// 	}
// 	_clients[clientFd].reset();
// 	return;
//   }
// }

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

void	ServerManager::loopReadyEvents(void) {
	int	fd;

	for (int i = 0; i < _readyEvents; i++) {
		fd = _requestBuf[i].data.fd;
		if (_servers.find(fd) != _servers.end()) {
			_servers.at(fd).handleServerEvent(_ServersClientsFds);
		}
		else {
			_servers.at(fd).handleClientEvent(fd); // TODO make this server specific. map clients to servers somehow
			// parseHttpRequest();
			// this->request.print();
			// handleCGI();
		}
	}
}
