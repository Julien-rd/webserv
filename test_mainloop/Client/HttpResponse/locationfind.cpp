#include "../../headers/structs/ServerStructs.hpp"
#include "../Client.hpp"
#include "HttpResponse.hpp"
#include <cstddef>
#include <fstream>
#include <iostream>
#include <memory>
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
  if (!locations.at(index).alias.empty())
    path = locations.at(index).alias +
           match.substr(locations.at(index).name.length());
  else if (!locations.at(index).root.empty())
    path = locations.at(index).root + match;
  else
    path = match;
  return index;
}

void processURI(const std::string& uri, int& httpCode, std::fstream& content,
                const std::vector<t_location>& locations) {
  std::string path;
  httpCode = 0;
  int index = getLocation(path, uri, locations);
  path.insert(0, "../../mySites");
  if (!locations.at(index)
           .redirect.second
           .empty()) { // fix: there are several redirects which dont need a
                       // file to be opened so check for the specific codes
    content.open(path.append(locations.at(index).redirect.second).c_str(),
                 std::ios::in | std::ios::binary);
    httpCode = locations.at(index).redirect.first;
  } else if (!locations.at(index).tryFiles.empty()) {
    for (size_t i = 0; i < locations.at(index).tryFiles.size(); ++i) {
      std::string tmp(path);
      content.open(tmp.append(locations.at(index).tryFiles.at(i)).c_str(),
                   std::ios::in | std::ios::binary);
      if (content.is_open())
        return;
    }
  } else if (!locations.at(index).index.empty()) {
    content.open(path.append(locations.at(index).index).c_str(),
                 std::ios::in | std::ios::binary);
  } else {
    std::cout << "opening {" << path << "/index.html" << "}\n";
    content.open(path.append("/index.html").c_str(),
                 std::ios::in | std::ios::binary);
  }
  if (!content.is_open()) {
    httpCode = -1;
  }
}
// introduce try_files, error_pages and return etc don't forget its actually
// first the return then try_files then index in precedence
