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
}   ;

enum d_tags {
    PORT,
    SERVER_NAME,
    INDEX,
    CLIENT_MAX_BODY_SIZE,
    ROOT,
    LISTEN
}   ;
static const std::vector<std::pair<std::string, int> > d = { {"port", PORT}, 
                                                {"server_name", SERVER_NAME}, {"index", INDEX},
                                                {"client_max_body_size", CLIENT_MAX_BODY_SIZE},
                                                {"root", ROOT}, {"listen", LISTEN}
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
    std::string root;
    std::string index;
    std::string location;
    std::string port;
    std::string client_max_body;
}   t_server;

typedef struct s_eval {
    bool    server, location;
    std::vector<t_server> servers;
}   t_eval;