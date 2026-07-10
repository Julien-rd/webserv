#pragma once
#include "../Client/Client.hpp"
#include "../ConfigParser/Structs.hpp"
#include "../Error/Error.hpp"
#include "../Utils/structs/ServerStructs.hpp"
#include "../Utils/typedefs.hpp"

#include <arpa/inet.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define BUFFER_SIZE 4096

typedef enum e_mapOperation { ADD, REMOVE } e_mapOperation;

class Server : public Error {
private:
  const t_config& _config;

  /* Attributes shared from Poller */
  int _epfd;

  /* Attributes shared from ServerManager  */
  int                    _sid;
  std::map<int, IntSet>& _serverToClientsMap;
  std::map<int, int>&    _clientToServerMap;
  std::map<int, Client>& _clients;

  /* Server's own attributes */
  int       _serverSocket;
  addrinfo* _addrInfo;

  void initServerSocket(void);
  void setServerSockAddr(void);
  void addSocketToEpoll(int socketFd);
  void bindAndListen(void);
  void setToNonBlocking(int socketFd);

  void closeConnection(int clientFd);
  void updateClientsMap(e_mapOperation op, const int clientFd);

public:
  Server(t_serverContext context);
  Server(const Server& obj);
  ~Server(void);

  int checkClientCap(void);
  void handleServerEvent(void);
  void handleClientEvent(int clientFd);

  int  start(void);
  void closeClientFds(void);

  int getIdentifier(void) const;
};
