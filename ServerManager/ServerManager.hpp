#pragma once
#include "../Server/Server.hpp"
#include "../Utils/structs/ServerManagerContext.hpp"
#include "../Utils/typedefs.hpp"

#include <map>
#include <vector>

#include <netinet/in.h>
#include <sys/epoll.h>

#define CLIENT_LIMIT 1024

class ServerManager : Error {
private:
  const t_config& _config;

  /* Attributes shared from Poller */
  int                       _epfd;
  const int&                _readyEventsCount;
  time_t                    _lastChecked;
  std::vector<epoll_event>& _triggeredEvents;

  /* ServerManager's own attributes */
  // Key: the fd of the server. Value: the server
  std::map<int, Server> _servers;
  // Key: the fd of the server. Value: all of its current clients
  std::map<int, IntSet> _serverToClientsMap;
  // Key: the fd of the client. Value: its owning server
  std::map<int, int> _clientToServerMap;
  // Key: the fd of the client. Value: the client
  std::map<int, Client> _clients;

  void addServerToMaps(int serverSocket, Server& server);
  void startServers(void);

public:
  ServerManager(const t_serverManagerContext& context);
  ~ServerManager(void);

  bool init(void);
  void    timeoutClients(void);
  void loopReadyEvents(void);
  int  matchClientToServer(int fd);
};
