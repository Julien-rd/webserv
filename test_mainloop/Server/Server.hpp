#ifndef SERVER_CLASS_HPP
#define SERVER_CLASS_HPP

#include "../Client/Client.hpp"
#include "../ConfigParser/Structs.hpp"
#include "../Error/Error.hpp"
#include "../headers/structs/ServerStructs.hpp"

#include <set>

#include <arpa/inet.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define BUFFER_SIZE 4096

enum e_map_operation { ADD, REMOVE };

class Server : public Error {
private:
  typedef std::set<int> IntSet;

  int                    _serverSocket;
  addrinfo*              _addrInfo;
  int                    _sid; // server identifier
  const t_config&        _config;
  int                    _epfd;
  std::map<int, IntSet>& _clientsMap;
  std::map<int, int>&    _clientToServerMap;
  std::map<int, Client>& _clients;

  void initServerSocket(void);
  void setServerSockAddr(void);
  void addSocketToEpoll(int socketFd);
  void bindAndListen(void);
  void setToNonBlocking(int socketFd);

  void closeConnection(int clientFd);
  void updateClientsMap(enum e_map_operation op, const int clientFd);

public:
  Server(t_serverContext context);
  Server(const Server& obj);
  ~Server(void);

  void checkClientCap(void);
  void handleServerEvent(void);
  void handleClientEvent(int clientFd);

  int  start(void);
  void closeClientFds(void);

  int getIdentifier(void) const;
};

#endif /* SERVER_CLASS_HPP */
