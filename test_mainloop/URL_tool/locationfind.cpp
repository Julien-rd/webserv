# include "../Client/Client.hpp" 
# include "../headers/structs/ServerStructs.hpp"
#include <iostream>
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

void getContent(const std::string& match, const std::vector<t_location>& locations) {
    unsigned int index = getDestination(match, locations);
    // JOB use the directives index, try_files, autoindex?, return? to give the correct full file  location back
    std::cout << "result: " << locations.at(index).name << "\n";
}

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