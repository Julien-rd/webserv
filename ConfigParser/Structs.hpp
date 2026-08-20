#pragma once

#include "../Logger/Logger.hpp"

#include <map>
#include <string>
#include <vector>

struct Node {
    int                      tag, type;
    std::vector<Node *>      content;
    std::vector<std::string> args;
};

enum types { CONTEXT, DIRECTIVE, BASE };

enum d_tags {
    ERRORPAGE,
    LOGLVL,
    INDEX,
    TRYFILES,
    CLIENT_MAX_BODY_SIZE,
    ROOT,
    ALIAS,
    LISTEN,
    AUTOINDEX,
    RETURN,
    ALLOWMETHODS,
    MAX_CLIENTS,
    CLIENTS_PER_SERVER,
    LOCALHOST,
    CGI_CONFIG,
    CLIENT_TIMEOUT
};

enum c_tags { SERVER, LOCATION, MAIN };

typedef struct c_cgi_config {
    std::string extension;
    std::string executablePath;
} t_cgi_config;

typedef struct s_location {
    std::string                 name;
    bool                        autoindex;
    std::string                 root;
    std::string                 index;
    std::string                 alias;
    std::vector<std::string>    tryFiles;
    std::pair<int, std::string> redirect;
    std::vector<std::string>    allowMethods;
} t_location;

typedef struct s_server {
    bool                      localhost;
    // std::string server_name;
    std::string               ip;
    std::string               port;
    std::map<int, std::string> errorPages;
    unsigned int              clientMaxBody;
    std::vector<t_location>   locations;
    std::vector<t_cgi_config> cgiConfigs;
} t_server;

typedef struct s_config {
    bool                  serverFlag, locationFlag;
    unsigned int          maxClients;
    unsigned int          clientTimeout;
    Level::Value          logLvl;
    // int errorLog;
    // int errorlvl;
    unsigned int          clientsPerServer;
    // int totalServerAmount;
    std::vector<t_server> servers;
} t_config;
