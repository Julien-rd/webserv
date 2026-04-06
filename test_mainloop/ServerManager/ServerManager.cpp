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
_config(config) {}

ServerManager::~ServerManager(void) {
	for (std::map<int, Server>::iterator it = _serversMap.begin(); it != _serversMap.end(); it++) {
		it->second.closeClientFds();
	}
}

void	ServerManager::validateServerConfig(const t_server_config config) const {
	(void)config;
	if (false) { // TODO Implement
		throw std::exception();
	}
}

void	ServerManager::validateConfig() const { // TODO Implement
	size_t	totalClients = 0;

	for (size_t i = 0; i < _config.serverConfigs.size(); i++) {
		if (_config.serverConfigs[i].maxClients > SERVER_CLIENT_LIMIT) {
			std::cerr << "ERROR: too many clients in a sever" << std::endl;
			throw std::exception();
		}
		totalClients += _config.serverConfigs[i].maxClients;
		if (totalClients > TOTAL_CLIENT_LIMIT) {
			std::cerr << "ERROR: too many total clients" << std::endl;
			throw std::exception();
		}
	}
	// TODO Maybe limit maximum servers in config parser???
}

void	ServerManager::createEpoll(void) {
	_epfd = epoll_create1(0);
	if (_epfd == -1) {
		error_msg(ERR_EPOLL_CREATE1);
		throw std::exception();
	}
}

void	ServerManager::setServerMaps(int serverSocket, Server& server) {
	_serversMap.insert(std::pair<int, Server>(serverSocket, server));
	_clientsMap.insert(std::pair<int, IntSet>(serverSocket, IntSet()));
}

void	ServerManager::startServers() {
	int		serverSocket;

	for (size_t i = 0; i < _config.serverConfigs.size(); i++) {
		Server	server(_config.serverConfigs[i], _epfd, _clientsMap);
		try {
			validateServerConfig(_config.serverConfigs[i]);
			serverSocket = server.start();
		}
		catch (std::exception& e) {
			std::cerr << "ERROR: Couldn't start server: " << e.what() << std::endl;
			continue ;
		}
		setServerMaps(serverSocket, server);
		std::cout << "Started Server __" << _serversMap.at(serverSocket).getName() <<"__ with socket " <<  serverSocket << " successfully" << std::endl;		
	}
}

void	ServerManager::initRequestBuf(void) {
	size_t	totalClients = 0;

	for (size_t i = 0; i < _config.serverConfigs.size(); i++) {
		totalClients += _config.serverConfigs[i].maxClients;
	}
	_requestBuf.reserve(totalClients + _config.serverConfigs.size());
	_maxEvents = totalClients + _config.serverConfigs.size();
}

void	ServerManager::init(void) {
	validateConfig();
	initRequestBuf();
	createEpoll();
	startServers();
	// std::cout << "map 4:" << _serversMap.at(4).getName() << " FD: " << _serversMap.at(4).getServerSocket() << std::endl;
}

void	ServerManager::epollWait() {
	// std::cout << "calling epoll_wait(" << _epfd << ", " << _requestBuf.data() << ", " << CLIENTS << ", " << -1 << ")" << std::endl;
	_readyEvents = epoll_wait(_epfd, _requestBuf.data(), _maxEvents, -1); // FIXME hardcode
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
	for (std::map<int, IntSet>::iterator it = _clientsMap.begin(); it != _clientsMap.end(); it++) {
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
		if (_serversMap.find(fd) != _serversMap.end()) {
			_serversMap.at(fd).handleServerEvent();
		}
		else {
			int	serverFd = matchClientToServer(fd);
			if (serverFd == -1) {
				std::cerr << "something wrong happened, couldn't match client to any server";
				continue ;
			}
			_serversMap.at(serverFd).handleClientEvent(fd);
			// parseHttpRequest();
			// this->request.print();
			// handleCGI();
		}
	}
}
