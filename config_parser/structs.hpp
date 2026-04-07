#pragma once

#include <vector>
#include <string>

struct Node { 
    int tag, type;
    std::vector<Node *> content;
    std::vector<std::string> args;
}   ;

typedef struct s_location {
    std::string location;
    std::string autoindex;
    std::string root;
    std::string index;
    
    int allowMethods;
}   t_location;

enum types {
    CONTEXT,
    DIRECTIVE,
    BASE
}   ;

enum d_tags {
    PORT,
    SERVER_NAME,
    INDEX,
    CLIENT_MAX_BODY_SIZE,
    ROOT,
    LISTEN,
    AUTOINDEX,
    ALLOWMETHODS,
}   ;

static const std::vector<std::pair<std::string, int> > d = { {"port", PORT}, 
                                                {"server_name", SERVER_NAME}, {"index", INDEX},
                                                {"client_max_body_size", CLIENT_MAX_BODY_SIZE},
                                                {"root", ROOT}, {"listen", LISTEN}, {"autoindex", AUTOINDEX}, 
                                                {"allow_methods", ALLOWMETHODS},
                                                }   ;

enum c_tags {
    SERVER,
    LOCATION,
    MAIN
}   ;
static const std::vector<std::pair<std::string, int> > c = {{"server", SERVER}, {"location", LOCATION} };

typedef struct s_server {
    bool localhost;
    std::string server_name;
    std::string listen;
    std::string port;
    std::string root;
    std::vector<t_location> locations;
    std::string client_max_body;
}   t_server;

typedef struct s_eval {
    bool    server, location;
    std::vector<t_server> servers;
}   t_eval;


typedef struct s_conf {
    int maxClients;
    int errorLog;
    int errorlvl;
    int clientsPerServer;
    int totalServerAmount;
    std::vector<t_server> servers;
}   t_conf;