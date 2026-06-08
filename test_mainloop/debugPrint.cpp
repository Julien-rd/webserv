#include "ConfigParser/Structs.hpp"
#include <iostream>
#include <string>
#include <vector>

static void printCgiConfig(const t_cgi_config& cgi, const std::string& indent) {
  std::cout << indent << "extension:      " << cgi.extension << "\n";
  std::cout << indent << "executablePath: " << cgi.executablePath << "\n";
}

static void printLocation(const t_location& loc, const std::string& indent) {
  std::cout << indent << "name:        " << loc.name << "\n";
  std::cout << indent << "autoindex:   " << (loc.autoindex ? "true" : "false")
            << "\n";
  std::cout << indent << "root:        " << loc.root << "\n";
  std::cout << indent << "index:       " << loc.index << "\n";
  std::cout << indent << "alias:       " << loc.alias << "\n";
  std::cout << indent << "allowMethods:" << "\n";
  for (unsigned int i = 0; i < loc.allowMethods.size(); ++i)
      std::cout << loc.allowMethods.at(i) << "\n";
  std::cout << indent << "redirect:    [" << loc.redirect.first << "] "
            << loc.redirect.second << "\n";

  std::cout << indent << "tryFiles:    [";
  for (size_t i = 0; i < loc.tryFiles.size(); i++) {
    if (i)
      std::cout << ", ";
    std::cout << loc.tryFiles[i];
  }
  std::cout << "]\n";

}

static void printServer(const t_server& srv, size_t idx) {
  std::cout << "  server[" << idx << "]:\n";
  std::cout << "    localhost:       " << (srv.localhost ? "true" : "false")
            << "\n";
  std::cout << "    ip:              " << srv.ip << "\n";
  std::cout << "    port:            " << srv.port << "\n";
  std::cout << "    client_max_body: " << srv.client_max_body << "\n";

  if (!srv.cgiConfigs.empty()) {
    std::cout << "    cgiConfigs (" << srv.cgiConfigs.size() << "):\n";
    for (size_t i = 0; i < srv.cgiConfigs.size(); i++) {
      std::cout << "      [" << i << "]:\n";
      printCgiConfig(srv.cgiConfigs[i], "        ");
    }
  }

  if (!srv.locations.empty()) {
    std::cout << "    locations (" << srv.locations.size() << "):\n";
    for (size_t i = 0; i < srv.locations.size(); i++) {
      std::cout << "      location[" << i << "]:\n";
      printLocation(srv.locations[i], "        ");
    }
  }
}

void printConfig(const t_config& cfg) {
  std::cout << "=== Config ===\n";
  std::cout << "  serverFlag:       " << (cfg.serverFlag ? "true" : "false")
            << "\n";
  std::cout << "  locationFlag:     " << (cfg.locationFlag ? "true" : "false")
            << "\n";
  std::cout << "  maxClients:       " << cfg.maxClients << "\n";
  std::cout << "  clientsPerServer: " << cfg.clientsPerServer << "\n";

  std::cout << "  servers (" << cfg.servers.size() << "):\n";
  for (size_t i = 0; i < cfg.servers.size(); i++)
    printServer(cfg.servers[i], i);

  std::cout << "=== End Config ===\n";
}
