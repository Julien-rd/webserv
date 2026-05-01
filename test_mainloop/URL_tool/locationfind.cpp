# include "../Client/Client.hpp" 
# include "../headers/structs/ErrorType.hpp"
# include "../headers/structs/ServerStructs.hpp"
# include "../ParseConfig/Structs.hpp"

unsigned int getDestination(const std::string& match, const std::vector<t_location>& locations) {
    unsigned int longestMatch = 0;
    int diff;
    unsigned int ret = 0;
    for (unsigned int i = 0; i < locations.size(); ++i) {
        diff = locations.at(0).name.compare(match);
        if (diff > longestMatch && locations.at(0).name.size() >= match.size()) {
            longestMatch = diff;
            ret = i;
        }
    }
    return ret;
}

const std::string& getContent(const std::string& url, const std::vector<t_location>& locations) {
    std::string match = url.substr(url.find('/')); // FIX what does the URL really look like is it with e.g. http://
    unsigned int index = getDestination(match, locations);
    // JOB use the directives index, try_files, autoindex?, return? to give the correct full file  location back
}