#pragma once
#include "Structs.hpp"
#include "Tokenizer.hpp"

#define GET 0
#define POST 1
#define DELETE 2

/* DirectiveHandlers.cpp */
void parseAutoindex(const std::vector<std::string> &args, t_location &location);
void parseLocalhost(const std::vector<std::string> &args, t_server &server);
void parseNumberGlobal(const std::vector<std::string> &args,
                       unsigned int *number);
void parseMaxBody(const std::vector<std::string> &args, t_server &server);
void parseListen(const std::vector<std::string> &args, t_server &server);
int allowMethods(std::vector<std::string> &args);
void parseRoot(const std::vector<std::string> &args, t_location &location);
void parseIndex(const std::vector<std::string> &args, t_location &location);

/*Globals */
Node *parseTree(Tokenizer &stream);
void evalTree(Node *tree, t_config &evalData);
bool parseConfigFile(t_config &config, const char *fileName);
