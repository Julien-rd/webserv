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

void	ServerManager::addSocketToEpoll(int socketFd) {
	struct epoll_event	ev;
	ev.events = EPOLLIN;
	ev.data.fd = socketFd;
	// std::cout << "adding " << socketFd << " to epoll " << _epfd << std::endl;
	if (epoll_ctl(_epfd, EPOLL_CTL_ADD, socketFd, &ev) == -1) {
		error_msg(ERR_EPOLL_CTL);
		throw std::exception();
	}
}

void	ServerManager::startServers() {
	int		serverSocket;

	for (size_t i = 0; i < _config.serverConfigs.size(); i++) {
		Server	server(_config.serverConfigs[i], _epfd);
		try {
			validateServerConfig(_config.serverConfigs[i]);
			serverSocket = server.start();
		}
		catch (std::exception& e) {
			std::cerr << "ERROR: Couldn't start server: " << e.what() << std::endl;
			continue ;
		}
		_servers.insert(std::pair<int, Server>(serverSocket, server));
		_serversClientsFds.insert(std::pair<int, IntSet>(serverSocket, IntSet()));
		addSocketToEpoll(serverSocket);
		std::cout << "Started Server " << serverSocket << " __" << _servers.at(serverSocket).getName() << "__ successfully" << std::endl;
		// _servers.at(serverSocket) = server;
		// _serversFds.insert(serverSocket);
		
	}
}

void	ServerManager::init(void) {
	validateConfig();
	createEpoll();
	startServers();
	// std::cout << "map 4:" << _servers.at(4).getName() << " FD: " << _servers.at(4).getServerSocket() << std::endl;
}

void	ServerManager::epollWait() {
	// std::cout << "calling epoll_wait(" << _epfd << ", " << _requestBuf.data() << ", " << CLIENTS << ", " << -1 << ")" << std::endl;
	_readyEvents = epoll_wait(_epfd, _requestBuf.data(), CLIENTS, -1); // FIXME hardcode
	// if (gSignalStatus)
	//   break;
	if (_readyEvents == -1) {
		error_msg(ERR_EPOLL_WAIT);
		perror(strerror(errno));
		throw std::exception();
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

int		ServerManager::matchClientToServer(int fd) {
	for (std::map<int, IntSet>::iterator it = _serversClientsFds.begin(); it != _serversClientsFds.end(); it++) {
		if (it->second.find(fd) != it->second.end()) {
			return it->first;
		}
	}
	return -1;
}

void	ServerManager::loopReadyEvents(void) {
	int	fd;

	for (int i = 0; i < _readyEvents; i++) {
		fd = _requestBuf[i].data.fd;
		if (_servers.find(fd) != _servers.end()) {
			std::cout << "fd is " << fd << std::endl;
			_servers.at(fd).handleServerEvent(_serversClientsFds);
		}
		else {
			int	serverFd = matchClientToServer(fd);
			if (serverFd == -1) {
				std::cerr << "something wrong happened, couldn't match client to any server";
				continue ;
			}
			_servers.at(serverFd).handleClientEvent(fd);
			// parseHttpRequest();
			// this->request.print();
			// handleCGI();
		}
	}
}
