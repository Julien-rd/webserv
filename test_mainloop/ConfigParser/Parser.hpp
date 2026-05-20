#pragma once
#include "Structs.hpp"
#include "Tokenizer.hpp"

#define GET 0
#define POST 1
#define DELETE 2

/* DirectiveHandlers.cpp */
void parseReturn(const std::vector<std::string>& args, t_location& location);
void parseTryFiles(const std::vector<std::string>& args, t_location& location);
void parseAlias(const std::vector<std::string>& args, t_location& location);
void parseAutoindex(const std::vector<std::string>& args, t_location& location);
void parseLocalhost(const std::vector<std::string>& args, t_server& server);
void parseNumberGlobal(const std::vector<std::string>& args,
                       unsigned int*                   number);
void parseMaxBody(const std::vector<std::string>& args, t_server& server);
void parseListen(const std::vector<std::string>& args, t_server& server);
int  allowMethods(std::vector<std::string>& args);
void parseRoot(const std::vector<std::string>& args, t_location& location);
void parseIndex(const std::vector<std::string>& args, t_location& location);

/*Globals */
Node* parseTree(Tokenizer& stream);
void  evalTree(Node* tree, t_config& evalData);
bool  parseConfigFile(t_config& config, const char* fileName);
template <typename T>
void parseCGIConfig(const std::vector<std::string>& args, T& context) {
  if (args.size() < 2) {
    throw std::runtime_error("cgi_config needs mroe arguments");
  }
  for (size_t i = 1; i < context.cgiConfigs.size(); i++) {
    if (context.cgiConfigs.at(i).extension ==
        context.cgiConfigs.at(0).extension) {
      throw std::runtime_error("duplicate cgi extensions");
    }
  }
  t_cgi_config cgiConfig;
  cgiConfig.extension = args.at(0);
  cgiConfig.executablePath = args.at(1);
  if (args.size() == 3) {
    std::vector<std::string> methodArgs(args.begin() + 2, args.end());
    cgiConfig.allowedMethods = allowMethods(methodArgs);
  } else {
    cgiConfig.allowedMethods = (1 << GET) | (1 << POST) | (1 << DELETE);
  }
  context.cgiConfigs.push_back(cgiConfig);
}
