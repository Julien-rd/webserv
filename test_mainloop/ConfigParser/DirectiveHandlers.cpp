#include "Parser.hpp"
#include <algorithm>

void parseAutoindex(const std::vector<std::string>& args,
                    t_location&                     location) {
  if (args.size() != 1)
    throw std::runtime_error("autoindex directive invalid");
  if (args.at(0) == "on")
    location.autoindex = true;
  else if (args.at(0) == "off")
    location.autoindex = false;
  else
    throw std::runtime_error("autoindex directive invalid");
}

void parseLocalhost(const std::vector<std::string>& args, t_server& server) {
  if (args.size() != 1)
    throw std::runtime_error("localhost directive invalid");
  if (args[0] == "yes")
    server.localhost = true;
}

void parseNumberGlobal(const std::vector<std::string>& args,
                       unsigned int*                   number) {
  char* end;
  long  val = strtol(args.at(0).c_str(), &end, 10);
  if (*end != 0 || val < 0 || val > 5000 || args.size() != 1)
    throw std::runtime_error("maxclients directive invalid");
  *number = (int)val;
}

void parseMaxBody(const std::vector<std::string>& args, t_server& server) {
  char* end;
  long  val = strtol(args.at(0).c_str(), &end, 10);
  if (*end != 0 || val < 0 || val > 100 || args.size() != 1)
    throw std::runtime_error("client_max_body directive invalid");
  server.client_max_body = (int)val;
}

void parseListen(const std::vector<std::string>& args, t_server& server) {
  if (args.size() != 1) {
    throw std::runtime_error("listen directive invalid");
  }
  size_t      colon = args.at(0).find(':');
  const char* convert;
  if (colon != std::string::npos) { // "ip:port" form
    server.ip = args.at(0).substr(0, colon);
    convert = args.at(0).c_str() + colon + 1;
  } else if (args.at(0).find_first_not_of("0123456789") ==
             std::string::npos) { // "port" form
    server.ip = "0.0.0.0";
    convert = args.at(0).c_str();
    return;
  } else { // "ip" form
    server.ip = args.at(0);
    return;
  }
  char* end;
  long  val = strtol(convert, &end, 10);
  if (*end != 0 || end == convert || val < 0 || val > 65535)
    throw std::runtime_error("listen directive invalid");
  server.port = std::string(convert);
}

int allowMethods(std::vector<std::string>& args) {
  unsigned int count = 0;
  int          bitmap = 0;
  if (std::find(args.begin(), args.end(), "GET") != args.end()) {
    bitmap |= 1 << GET;
    ++count;
  }
  if (std::find(args.begin(), args.end(), "POST") != args.end()) {
    bitmap |= 1 << POST;
    ++count;
  }
  if (std::find(args.begin(), args.end(), "DELETE") != args.end()) {
    bitmap |= 1 << DELETE;
    ++count;
  }
  if (args.size() == 0 || args.size() != count)
    throw std::runtime_error("allow_methods has wrong arguments");
  return bitmap;
}

void parseRoot(const std::vector<std::string>& args, t_location& location) {
  if (args.size() != 1)
    throw std::runtime_error("root has wrong argument count");
  location.root = args.at(0);
}

void parseIndex(const std::vector<std::string>& args, t_location& location) {
  if (args.size() != 1)
    throw std::runtime_error("index has wrong argument count");
  location.index = args.at(0);
}

void parseAlias(const std::vector<std::string>& args, t_location& location) {
  if (args.size() != 1)
    throw std::runtime_error("alias has wrong argument count");
  location.alias = args.at(0);
}

void parseReturn(const std::vector<std::string>& args, t_location& location) {
  if (args.size() != 2)
    throw std::runtime_error("return has wrong argument count");
  char* end;
  long  val = strtol(args.at(0).c_str(), &end, 10);
  if (*end != 0 || end == args.at(0).c_str() || val < 0 || val > 500)
    throw std::runtime_error("return directive invalid");
  location.redirect.first = (int)val;
  location.redirect.second = args.at(1);
}
