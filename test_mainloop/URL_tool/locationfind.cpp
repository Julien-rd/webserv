# include "../Client/Client.hpp" 
# include "../headers/structs/ServerStructs.hpp"
#include <cstddef>
#include <iostream>
#include <memory>
#include <string>

unsigned int getDestination(const std::string& match, const std::vector<t_location>& locations) {
    unsigned int longestMatch = 0;
    int diff;
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


void getContent(std::string& match, std::vector<t_location>& locations) {
    // std::pair<int, std::vector<std::string>> ret;
    unsigned int index = getDestination(match, locations);
    if (!locations.at(index).alias.empty())
        match = locations.at(index).alias + match.substr(locations.at(index).name.length());
    else if (!locations.at(index).root.empty())
        match = locations.at(index).root + match;
}

//introduce try_files, error_pages and return etc don't forget its actually first the return then try_files then index in precedence
int main(int ac, char **argv) {
    std::vector<t_location> locations;
    t_location one;
    one.name = "/hello/what";
    t_location two;
    two.name= "/hello/what/sdf";
    t_location three;
    three.name = "/";
    locations.push_back(one);
    locations.push_back(two);
    locations.push_back(three);
    std::string ye = argv[1];
    getContent(ye, locations);
}