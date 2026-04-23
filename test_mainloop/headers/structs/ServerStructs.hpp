#pragma once

typedef struct s_serverContext {
    int epfd;
    unsigned int maxClients;
    unsigned int clientsPerServer;
} t_serverContext;
