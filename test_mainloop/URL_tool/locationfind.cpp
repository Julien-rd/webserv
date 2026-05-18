# include "../Client/Client.hpp" 
# include "../headers/structs/ServerStructs.hpp"
#include <cstddef>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>

unsigned int getDestination(const std::string& match, const std::vector<t_location>& locations) {
    unsigned int longestMatch = 0;
    unsigned int ret = 0;
    
    std::cout << locations.size() << std::endl;
    for (unsigned int i = 0; i < locations.size(); ++i) {
        std::string identifier = locations.at(i).name;
        if (match.compare(0, identifier.size(), identifier) == 0 && identifier.size() > longestMatch) {
            longestMatch = identifier.size();
            ret = i;
        }
    }
    return ret;
}

int getLocation(std::string& path, const std::string& match, std::vector<t_location>& locations) {
    unsigned int index = getDestination(match, locations);
    if (!locations.at(index).alias.empty())
        path = locations.at(index).alias + match.substr(locations.at(index).name.length());
    else if (!locations.at(index).root.empty())
        path = locations.at(index).root + match;
    else
        path = match;
    return index;
}

void    processURI(const std::string& uri, int &htmlCode, std::ifstream& content, std::vector<t_location>& locations) {
    std::string path;
    htmlCode = 0;
    int index = getLocation(path, uri, locations);
    if (!locations.at(index).redirect.second.empty()) { //fix: there are several redirects which dont need a file to be opened so check for the specific codes
        content.open(path + locations.at(index).redirect.second);
        htmlCode = locations.at(index).redirect.first;
    }
    else if (!locations.at(index).tryFiles.empty()) {
        while (!locations.at(index).tryFiles.empty()) {
            content.open(path + locations.at(index).tryFiles.back());
            if (content.is_open())
                return ;
            locations.at(index).tryFiles.pop_back();
        }
    }
    else if (!locations.at(index).index.empty()) {
        content.open(path + locations.at(index).index);
    }
    else {
        content.open(path + "/index.html");
    }
    if (!content.is_open())
        htmlCode = -1;
}
//introduce try_files, error_pages and return etc don't forget its actually first the return then try_files then index in precedence