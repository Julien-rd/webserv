#ifndef SERVER_MANAGER_CLASS_HPP
#define SERVER_MANAGER_CLASS_HPP

#include "../Server/Server.hpp"
#include "../headers/structs/ServerManagerContext.hpp"
#include "../headers/typedefs.hpp"

#include <map>
#include <vector>

#include <netinet/in.h>
#include <sys/epoll.h>

#define CLIENT_LIMIT 1024

class ServerManager : Error {
private:
  const t_config&           _config;
  int                       _epfd;
  const int&                _readyEventsCount;
  std::vector<epoll_event>& _triggeredEvents;
  // Key: the fd of the server. Value: the server
  std::map<int, Server> _serversMap;
  // Key: the fd of the server. Value: all of its current clients
  std::map<int, IntSet> _clientsMap;
  // Key: the fd of the client. Value: its owning server
  std::map<int, int> _clientToServerMap;

  std::map<int, Client> _clients;

  void addServerToMaps(int serverSocket, Server& server);
  void startServers(void);

public:
  ServerManager(const t_serverManagerContext& context);
  ~ServerManager(void);

  bool init(void);
  void loopReadyEvents(void);
  int  matchClientToServer(int fd);
};

#endif /* SERVER_MANAGER_CLASS_HPP */
