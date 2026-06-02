#include "../../headers/structs/ServerStructs.hpp"
#include "../Client.hpp"
#include "HttpResponse.hpp"
#include <cstddef>
#include <fstream>
#include <iostream>
#include <string>

unsigned int getDestination(const std::string&             match,
                            const std::vector<t_location>& locations) {
  unsigned int longestMatch = 0;
  unsigned int ret = 0;

  for (unsigned int i = 0; i < locations.size(); ++i) {
    std::string identifier = locations.at(i).name;
    if (match.compare(0, identifier.size(), identifier) == 0 &&
        identifier.size() > longestMatch) {
      longestMatch = identifier.size();
      ret = i;
    }
  }
  return ret;
}

int getLocation(std::string& path, const std::string& match,
                const std::vector<t_location>& locations) {
  unsigned int index = getDestination(match, locations);
  if (!locations.at(index).alias.empty()) {
    path = locations.at(index).alias +
           match.substr(locations.at(index).name.length()); 
  }
  else if (!locations.at(index).root.empty())
    path = locations.at(index).root + match;
  else
    path = locations.at(index).name;
  return index;
}

void printLocations(const std::vector<t_location>& locations) {
  std::cout << "\n\ncalling printLocations() in processURI()\n";
  for (size_t i = 0; i < locations.size(); i++) {
    std::cout << "location: " << i << "\n";
    std::cout << "alias: " << locations.at(i).alias << "\n";
    // std::cout << "allowMethods: " << locations.at(i).allowMethods << "\n";
    std::cout << "autoindex: " << locations.at(i).autoindex << "\n";
    // std::cout << "cgiConfigs execPath: "
    //           << locations.at(i).cgiConfigs.at(i).executablePath << "\n";
    std::cout << "index: " << locations.at(i).index << "\n";
    std::cout << "name: " << locations.at(i).name << "\n";
    std::cout << "redirect: " << locations.at(i).redirect.first
              << " ==== second: " << locations.at(i).redirect.second << "\n";
    std::cout << "root: " << locations.at(i).root << "\n";
    std::cout << "tryFiles: ";
    for (size_t j = 0; j < locations.at(i).tryFiles.size(); j++) {
      std::cout << locations.at(i).tryFiles.at(j) << " == ";
    }
    std::cout << "\n";
    std::cout << "-------------------------------------------------------\n";
  }
  std::cout << "=======================================================\n";
}

std::string processURI(const std::string& uri, int& httpCode, std::fstream& content,
                const std::vector<t_location>& locations) {
  printLocations(locations);
  std::string path;
  httpCode = 0;
  int index = getLocation(path, uri, locations);
  path.insert(0, "../mySites");
  if (!locations.at(index)
           .redirect.second
           .empty()) { // fix: there are several redirects which dont need a
                       // file to be opened so check for the specific codes
    std::cout << "path: {{" << path << "}} ====== append redirect: {{"
              << locations.at(index).redirect.second << "}}\n";
    content.open(path.append(locations.at(index).redirect.second).c_str(),
                 std::ios::in | std::ios::binary);
    httpCode = locations.at(index).redirect.first;
  } else if (!locations.at(index).tryFiles.empty()) {
    for (size_t i = 0; i < locations.at(index).tryFiles.size(); ++i) {
      std::string tmp(path);
      std::cout << "tmp: {{" << tmp << "}} ====== append tryFiles: {{"
                << locations.at(index).tryFiles.at(i) << "}}\n";
            
      content.open(tmp.append(locations.at(index).tryFiles.at(i)).c_str(),
                   std::ios::in | std::ios::binary);
      if (content.is_open())
        return tmp;
    }
  } else if (!locations.at(index).index.empty()) {
      std::string tmp2(path);
    std::cout << "path: {{" << path << "}} ====== append index: {{"
              << locations.at(index).index << "}}\n";
    std::cout << std::endl << tmp2.append(locations.at(index).index) << "\n";
    content.open(path.append(locations.at(index).index).c_str(),
                 std::ios::in | std::ios::binary);
  } else {
    std::cout << "path: {{" << path << "}} ====== append default: {{"
              << locations.at(index).index << "}}\n";
    content.open(path.append("/index.html").c_str(),
                 std::ios::in | std::ios::binary);
  }
  if (!content.is_open()) {
    httpCode = -1;
  }
  return path;
}
//   error_pages and return etc don't forget its actually
// first the return then try_files then index in precedence
