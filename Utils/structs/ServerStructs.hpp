#pragma once

#include "../../Client/Client.hpp"
#include "../../ConfigParser/Structs.hpp"
#include "../typedefs.hpp"

#include <map>

typedef struct {
    int                    epfd;
    const t_config        &config;
    size_t                 sid;
    std::map<int, IntSet> &serverToClientsMap;
    std::map<int, Client> &clients;
    std::map<int, int>    &clientToServerMap;
    // unsigned int maxClients;
    // unsigned int clientsPerServer;
} t_serverContext;
