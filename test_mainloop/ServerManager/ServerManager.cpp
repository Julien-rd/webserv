#include "ServerManager.hpp"
#include "../headers/structs/ServerStructs.hpp"

#include <exception>
#include <iostream>
#include <sstream>
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
  for (std::map<int, Server>::iterator it = _serversMap.begin();
       it != _serversMap.end(); it++) {
    it->second.closeClientFds();
    close(it->first);
  }
}

void ServerManager::addServerToMaps(int serverSocket, Server& server) {
  _serversMap.insert(std::pair<int, Server>(serverSocket, server));
  _clientsMap.insert(std::pair<int, IntSet>(serverSocket, IntSet()));
}

void ServerManager::startServers(void) {
  int serverSocket;

  for (size_t i = 0; i < _config.servers.size(); ++i) {
    t_serverContext context = {_epfd,       _config,  i,
                               _clientsMap, _clients, _clientToServerMap};
    Server          server(context);
    try {
      serverSocket = server.start();
    } catch (std::exception& e) {
      std::cerr << "WARNING: couldn't start server _" << server.getIdentifier()
                << "_: " << e.what() << std::endl;
      continue;
    }
    addServerToMaps(serverSocket, server);
    std::cout << "Started Server __"
              << _serversMap.at(serverSocket).getIdentifier()
              << "__ with socket " << serverSocket << " successfully"
              << std::endl;
  }
}

bool ServerManager::init(void) {
  startServers();
  if (_serversMap.size() == 0) {
    std::cout << "WARNING: no servers were started" << std::endl;
    return 1;
  }
  std::cout << std::endl;
  return 0;
}

void ServerManager::loopReadyEvents(void) {
  for (int i = 0; i < _readyEventsCount; ++i) {
    int fd = _triggeredEvents[i]
                 .data.fd; // if it's Server or Client event, data union
                           // will have Server or Client fd in fd. if its a
                           // CGI event, data union will save two ints
                           // (pipefd & clientFd) in u64 (or ptr)
    std::cout << "fd in loop is: " << fd << std::endl;
    if (_serversMap.find(fd) != _serversMap.end()) {
      _serversMap.at(fd).handleServerEvent();
    } else if (_clientToServerMap.find(fd) != _clientToServerMap.end()) {
      int serverFd = _clientToServerMap[fd];
      _serversMap.at(serverFd).handleClientEvent(fd);
    } else { /* is CGI's pipe fd */
      int* fds = reinterpret_cast<int*>(
          &_triggeredEvents[i].data.u64); // fds[0] is the pipefd. fds[1] is
                                          // the owning client's fd.
      std::cout << "caught CGI in epoll... client fd: " << fds[1]
                << ". pipefd is: " << fds[0] << std::endl;
      _clients[fds[1]].handleCGIOutput(fds[0]);
    }
  }
}
