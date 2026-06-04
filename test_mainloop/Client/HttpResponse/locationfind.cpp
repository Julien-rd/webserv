#include "HttpResponse.hpp"
#include <cstddef>
#include <iostream>
#include <string>
#include <sys/stat.h>
#include <unistd.h>

bool isDirectory(std::string& path) {
  struct stat stats;
  stat(path.c_str(), &stats);
  if (S_ISDIR(stats.st_mode))
    return true;
  return false;
}

unsigned int getLocation(const std::string& match,
                         const t_server&    serverConfig) {
  unsigned int longestMatch = 0;
  unsigned int ret = 0;

  for (unsigned int i = 0; i < serverConfig.locations.size(); ++i) {
    std::string identifier = serverConfig.locations.at(i).name;
    if (match.compare(0, identifier.size(), identifier) == 0 &&
        identifier.size() > longestMatch) {
      longestMatch = identifier.size();
      ret = i;
    }
  }
  return ret;
}

void attachPrefix(const std::string& uri, std::string& path,
                  t_location& location) {
  if (!location.alias.empty())
    path = "../mySites" + location.alias + uri.substr(location.name.length());
  else if (!location.root.empty())
    path = "../mySites" + location.root + uri;
  else
    path = "../mySites" + uri;
}

// void printLocations(const std::vector<t_location>& locations) {
//     std::cout << "\n\ncalling printLocations() in processURI()\n";
//     for (size_t i = 0; i < locations.size(); i++) {
//         std::cout << "location: " << i << "\n";
//         std::cout << "alias: " << locations.at(i).alias << "\n";
//         // std::cout << "allowMethods: " << locations.at(i).allowMethods <<
//         "\n"; std::cout << "autoindex: " << locations.at(i).autoindex <<
//         "\n";
//         // std::cout << "cgiConfigs execPath: "
//         //           << locations.at(i).cgiConfigs.at(i).executablePath <<
//         "\n"; std::cout << "index: " << locations.at(i).index << "\n";
//         std::cout << "name: " << locations.at(i).name << "\n";
//         std::cout << "redirect: " << locations.at(i).redirect.first
//         << " ==== second: " << locations.at(i).redirect.second << "\n";
//         std::cout << "root: " << locations.at(i).root << "\n";
//         std::cout << "tryFiles: ";
//         for (size_t j = 0; j < locations.at(i).tryFiles.size(); j++) {
//             std::cout << locations.at(i).tryFiles.at(j) << " == ";
//             struct stat statbuf;
//             if (stat(fspath.c_str(), &statbuf) == -1) {
//                 // file doesn't exist or no permission → 404
//                 return;
//             }}
//         std::cout << "\n";
//         std::cout <<
//         "-------------------------------------------------------\n";
//     }
//     std::cout << "=======================================================\n";
// }

/**
 * @brief Uses the URI using config data to return the resolved path and http
 * code.
 * @param uri          The request URI from the client.
 * @param serverConfig The server configuration associated with this client.
 * @return UriResult - httpCode to determine action, path to execute action
 * with, bool for autoindex.
 */
UriResult processURI(const std::string& uri, const t_server& serverConfig) {
  UriResult   result;
  struct stat stats;

  result.httpCode = 200;
  result.autoindex = false;
  // printLocations(serverConfig.locations);
  unsigned int index = getLocation(uri, serverConfig);
  t_location   location = serverConfig.locations.at(index);
  if (!location.redirect.second.empty()) {
    result.httpCode = location.redirect.first;
    result.path = location.redirect.second;
    return result;
  }
  attachPrefix(uri, result.path, location);
  std::cout << "resolved path: [" << result.path << "]" << std::endl;
  if (stat(result.path.c_str(), &stats) == -1) {
    result.httpCode = 404;
  } else if (S_ISDIR(stats.st_mode)) {
    if (!uri.empty() && uri[uri.size() - 1] != '/') {
      result.httpCode = 301;
      result.path = uri + '/';
    } else if (!location.tryFiles.empty()) {
      std::string tmp;
      for (size_t i = 0; i < location.tryFiles.size(); ++i) {
        tmp = result.path + location.tryFiles.at(i);
        if (access(tmp.c_str(), F_OK | R_OK) == 0) {
          result.path = tmp;
          return result;
        }
      }
      result.httpCode = 404;
    } else if (!location.index.empty()) {
      result.path += location.index;
    } else if (access((result.path + "/index.html").c_str(), F_OK | R_OK) == 0)
      result.path += "/index.html";
    else if (location.autoindex)
      result.autoindex = true;
    else
      result.httpCode = 404;
  }
  return result;
}
