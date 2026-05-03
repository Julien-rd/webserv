#pragma once

#include "../../Client/Client.hpp"
#include "../../ConfigParser/Structs.hpp"
#include "../typedefs.hpp"

#include <map>

typedef struct s_serverContext {
  int                    epfd;
  const t_config&        config;
  size_t                 sid;
  std::map<int, IntSet>& clientsMap;
  std::map<int, Client>& clients;
  std::map<int, int>&    clientToServerMap;
  // unsigned int maxClients;
  // unsigned int clientsPerServer;
} t_serverContext;
