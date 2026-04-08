#include "ServerManager.hpp"

#include <exception>
#include <iostream>
#include <stdexcept>
#include <set>

#include <cerrno>
#include <csignal>
#include <cstdio>
#include <cstring>

#include <fcntl.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <sys/event.h>

ServerManager::ServerManager(const t_config& config):
_config(config) {
	_requestBuf.reserve(MAX_EVENTS);
}

ServerManager::~ServerManager(void) {
	close(_epfd);
	for (std::map<int, Server>::iterator it = _serversMap.begin(); it != _serversMap.end(); it++) {
		it->second.closeClientFds();
		close(it->first);
	}
}

void	ServerManager::validateServerConfig(const t_server_config config) const {
	if (config.host == 0) {
		throw std::runtime_error("host can't be 0 or unset");
	}
	if (config.name == "") {
		throw std::runtime_error("name can't be empty or unset");
	}
}

void	ServerManager::addServerToMaps(int serverSocket, Server& server) {
	_serversMap.insert(std::pair<int, Server>(serverSocket, server));
	_clientsMap.insert(std::pair<int, IntSet>(serverSocket, IntSet()));
}

void	ServerManager::validateConfig() const {
	if (_config.max_clients > CLIENT_LIMIT) {
		throw std::runtime_error("ERROR: too many max_clients");
	}
}

void	ServerManager::createEpoll(void) {
	_epfd = kqueue();
	if (_epfd == -1) {
		error_msg(ERR_EPOLL_CREATE1);
		throw std::exception();
	}
}

void	ServerManager::startServers(void) {
	int	serverSocket;

	for (size_t i = 0; i < _config.serverConfigs.size(); ++i) {
		Server	server(_config.serverConfigs[i], _epfd, _clientsMap, _clients);
		try {
			validateServerConfig(_config.serverConfigs[i]);
			serverSocket = server.start();
		}
		catch (std::exception& e) {
			std::cerr << "ERROR: Couldn't start server __" << _config.serverConfigs[i].name << "__: " << e.what() << std::endl;
			continue ;
		}
		addServerToMaps(serverSocket, server);
		std::cout << "Started Server __" << _serversMap.at(serverSocket).getName() <<"__ with socket " <<  serverSocket << " successfully" << std::endl;		
	}
}

bool	ServerManager::init(void) {
	try {
		validateConfig();
		createEpoll();
		startServers();
	}
	catch (std::exception& e) {
		std::cerr << e.what() << std::endl;
		return true;
	}
	return false;
}

void	ServerManager::epollWait() {
	_readyEvents = kevent(_epfd, NULL, 0, _requestBuf.data(), MAX_EVENTS, NULL);
	if (_readyEvents == -1) {
		error_msg(ERR_EPOLL_WAIT);
		perror(strerror(errno));
		throw std::exception();
	}
}

int		ServerManager::matchClientToServer(int clientFd) {
	for (std::map<int, IntSet>::iterator it = _clientsMap.begin(); it != _clientsMap.end(); ++it) {
		if (it->second.find(clientFd) != it->second.end()) {
			return it->first;
		}
	}
	throw std::runtime_error("ERROR: couldn't match event ClientFd to an existing entry in clientsMap");
}

void	ServerManager::loopReadyEvents(void) {
	int	fd;

	for (int i = 0; i < _readyEvents; ++i) {
		fd = _requestBuf[i].ident;
		if (_serversMap.find(fd) != _serversMap.end()) {
			_serversMap.at(fd).handleServerEvent();
		}
		else {
			int	serverFd = matchClientToServer(fd);
			_serversMap.at(serverFd).handleClientEvent(fd);
		}
	}
}
