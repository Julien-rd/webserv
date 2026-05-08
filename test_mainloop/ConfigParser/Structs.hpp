#pragma once

#include <string>
#include <vector>

struct Node {
  int tag, type;
  std::vector<Node *> content;
  std::vector<std::string> args;
};

enum types { CONTEXT, DIRECTIVE, BASE };

enum d_tags {
  INDEX,
  CLIENT_MAX_BODY_SIZE,
  ROOT,
  ALIAS,
  LISTEN,
  AUTOINDEX,
  RETURN, 
  ALLOWMETHODS,
  MAX_CLIENTS,
  CLIENTS_PER_SERVER,
  LOCALHOST
};

enum c_tags { SERVER, LOCATION, MAIN };

typedef struct s_location {
  std::string name;
  bool autoindex;
  std::string root;
  std::string index;
  std::string alias;
  std::pair<int, std::string> redirect;
  int allowMethods;
} t_location;

typedef struct s_server {
  bool localhost;
  // std::string server_name;
  std::string ip;
  std::string port;
  int client_max_body;
  std::vector<t_location> locations;
} t_server;

typedef struct s_config {
  bool serverFlag, locationFlag;
  unsigned int maxClients;
  // int errorLog;
  // int errorlvl;
  unsigned int clientsPerServer;
  // int totalServerAmount;
  std::vector<t_server> servers;
} t_config;
