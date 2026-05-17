#include "ServerManager.hpp"
#include "../headers/structs/ServerStructs.hpp"

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

ServerManager::ServerManager(const t_serverManagerContext& context)
    : _config(context.config), _epfd(context.epfd),
      _readyEventsCount(context.readyEventsCount),
      _triggeredEvents(context.triggeredEvents) {}

ServerManager::~ServerManager(void) {
  for (std::map<int, Server>::iterator it = _servers.begin();
       it != _servers.end(); it++) {
    it->second.closeClientFds();
    close(it->first);
  }
}

void ServerManager::addServerToMaps(int serverSocket, Server& server) {
  _servers.insert(std::pair<int, Server>(serverSocket, server));
  _serverToClientsMap.insert(std::pair<int, IntSet>(serverSocket, IntSet()));
}

void ServerManager::startServers(void) {
  int serverSocket;

  t_serverContext context = {
      _epfd, _config, 0, _serverToClientsMap, _clients, _clientToServerMap};
  for (size_t i = 0; i < _config.servers.size(); ++i) {
    context.sid = i;
    Server server(context);
    try {
      serverSocket = server.start();
    } catch (std::exception& e) {
      std::cerr << "WARNING: couldn't start server _" << server.getIdentifier()
                << "_: " << e.what() << std::endl;
      continue;
    }
    addServerToMaps(serverSocket, server);
    std::cout << "Started Server __"
              << _servers.at(serverSocket).getIdentifier() << "__ with socket "
              << serverSocket << " successfully" << std::endl;
  }
}

bool ServerManager::init(void) {
  startServers();
  if (_servers.size() == 0) {
    std::cout << "WARNING: no servers were started" << std::endl;
    return 1;
  }
  std::cout << std::endl;
  return 0;
}

void pp_memcpy(void* dst, void* src, size_t len) {
  for (size_t i = 0; i < len; i++) {
    static_cast<unsigned char*>(dst)[i] = static_cast<unsigned char*>(src)[i];
  }
}

void ServerManager::loopReadyEvents(void) {
  for (int i = 0; i < _readyEventsCount; ++i) {
    int fd = _triggeredEvents[i]
                 .data.fd; // if it's Server or Client event, data union
                           // will have Server or Client fd in fd. if its a
                           // CGI event, data union will save two ints
                           // (pipefd & clientFd) in u64 (or ptr)
    // std::cout << "fd in loop is: " << fd << std::endl;
    if (_servers.find(fd) != _servers.end()) {
      _servers.at(fd).handleServerEvent();
    } else if (_clientToServerMap.find(fd) != _clientToServerMap.end()) {
      _servers.at(_clientToServerMap[fd]).handleClientEvent(fd);
    } else {      /* is CGI's pipe fd */
      int fds[2]; // fds[0] is the pipefd. fds[1]
                  // is the owning client's fd.
      pp_memcpy(fds, &_triggeredEvents[i].data.u64, sizeof(uint64_t));
      // std::cout << "caught CGI in epoll... client fd: " << fds[1]
      //           << ". pipefd is: " << fds[0] << std::endl;
      _clients[fds[1]].handleCGIResponse(fds[0]);
    }
  }
}
