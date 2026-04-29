#include "ServerManager.hpp"
#include "../headers/structs/ServerStructs.hpp"

#include <exception>
#include <iostream>
#include <vector>
#include <sstream>
#include <fstream>

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

void	ServerManager::addServerToMaps(int serverSocket, Server& server) {
	_serversMap.insert(std::pair<int, Server>(serverSocket, server));
	_clientsMap.insert(std::pair<int, IntSet>(serverSocket, IntSet()));
}

void	ServerManager::createEpoll(void) {
	_epfd = epoll_create1(0);
	if (_epfd == -1) {
		error_msg(ERR_EPOLL_CREATE1);
		throw std::exception();
	}
}

void	ServerManager::startServers(void) {
	int	serverSocket;
	
	t_serverContext context = {_epfd, _config.maxClients, _config.clientsPerServer};
	for (size_t i = 0; i < _config.servers.size(); ++i) {
		Server	server(_config.servers[i], _clientsMap, _clients,
			_clientToServerMap, context, i); //INFO: server struct is now t_server if the name is not occupied already
		try {
			serverSocket = server.start();
		}
		catch (std::exception& e) {
			std::cerr << "WARNING: couldn't start server _" << server.getIdentifier() << "_: " << e.what() << std::endl;
			continue ;
		}
		addServerToMaps(serverSocket, server);
		std::cout << "Started Server __" << _serversMap.at(serverSocket).getIdentifier() <<"__ with socket " <<  serverSocket << " successfully" << std::endl;		
	}
}

bool	ServerManager::init(void) {
	try {
		createEpoll();
		startServers();
	}
	catch (std::exception& e) {
		std::cerr << e.what() << std::endl;
		return true;
	}
	std::cout << std::endl;
	return false;
}

void	ServerManager::epollWait() {
	_readyEvents = epoll_wait(_epfd, _requestBuf.data(), MAX_EVENTS, -1);
	if (_readyEvents == -1) {
		error_msg(ERR_EPOLL_WAIT);
		// perror(strerror(errno));
		throw std::exception();
	}
}

static void	PPMemcpy(void *dest, void *src, int size) {
	for (int i = 0; i < size; i++) {
		static_cast<unsigned char*>(dest)[i] = static_cast<unsigned char*>(src)[i];
	}
}

void	ServerManager::loopReadyEvents(void) {
	std::stringstream	ss;

	for (int i = 0; i < _readyEvents; ++i) {
		int	fd = _requestBuf[i].data.fd;
		// std::cout << "fd in loop is: " << fd << std::endl;
		if (_serversMap.find(fd) != _serversMap.end()) {
			_serversMap.at(fd).handleServerEvent();
		}
		else if (_clientToServerMap.find(fd) != _clientToServerMap.end()) {
			int	serverFd = _clientToServerMap[fd];
			_serversMap.at(serverFd).handleClientEvent(fd);
		}
		else { /* is CGI's pipe fd */
			int fds[2]; // fds[0] is the pipefd. fds[1] is the owning client's fd.
			PPMemcpy(fds, &_requestBuf[i].data.u64, sizeof(uint64_t));
			std::cout << "caught CGI in epoll... client fd: " << fds[1] << ". pipefd is: " << fds[0] << std::endl;
			char buf[1024];
			ssize_t bytes_read = read(fds[0], buf, 1023);
			buf[bytes_read] = 0;
			std::cout << "reading pipe: " << buf << std::endl;
		}
	}
}
