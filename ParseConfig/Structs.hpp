#pragma once

#include <vector>
#include <string>


struct Node { 
    int tag, type;
    std::vector<Node *> content;
    std::vector<std::string> args;
}   ;


enum types {
    CONTEXT,
    DIRECTIVE,
    BASE
}   ;

enum d_tags {
    SERVER_NAME,
    INDEX,
    CLIENT_MAX_BODY_SIZE,
    ROOT,
    LISTEN,
    AUTOINDEX,
    ALLOWMETHODS,
    MAX_CLIENTS,
    CLIENTS_PER_SERVER,
    LOCALHOST
}   ;

static const std::vector<std::pair<std::string, int> > d = {
                                                {"server_name", SERVER_NAME}, {"index", INDEX},
                                                {"client_max_body_size", CLIENT_MAX_BODY_SIZE},
                                                {"root", ROOT}, {"listen", LISTEN}, {"autoindex", AUTOINDEX}, 
                                                {"allow_methods", ALLOWMETHODS},
                                                {"max_clients", MAX_CLIENTS},
                                                {"clients_per_server", CLIENTS_PER_SERVER},
                                                {"localhost", LOCALHOST}
                                                }   ;

enum c_tags {
    SERVER,
    LOCATION,
    MAIN
}   ;
static const std::vector<std::pair<std::string, int> > c = {{"server", SERVER}, {"location", LOCATION} };

typedef struct s_location {
    std::string name;
    bool autoindex;
    std::string root;
    std::string index;
    
    int allowMethods;
}   t_location;

typedef struct s_server {
    bool localhost;
    // std::string server_name;
    std::string ip;
    int port;
    int client_max_body;
    std::vector<t_location> locations;
}   t_server;

typedef struct s_eval {
    bool    serverFlag, locationFlag;
    int maxClients;
    // int errorLog;
    // int errorlvl;
    int clientsPerServer;
    // int totalServerAmount;
    std::vector<t_server> servers;
}   t_eval;