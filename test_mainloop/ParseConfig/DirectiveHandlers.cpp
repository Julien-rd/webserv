#include "Parser.hpp"
#include <algorithm>

void    parseAutoindex(const std::vector<std::string> &args, t_location &location) {
    if (args.size() != 1)
        throw std::runtime_error("autoindex directive invalid");
    if (args[0] == "yes")
        location.autoindex = true;
}

void    parseLocalhost(const std::vector<std::string> &args, t_server &server) {
    if (args.size() != 1)
        throw std::runtime_error("localhost directive invalid");
    if (args[0] == "yes")
        server.localhost = true;
}

void    parseNumberGlobal(const std::vector<std::string> &args, int *number) {
    char *end;
    long val = strtol(args.at(0).c_str(),  &end, 10);
    if (*end != 0 || val < 0 || val > 5000 || args.size() != 1)
        throw std::runtime_error("maxclients directive invalid");
    *number = (int)val;
}

void    parseMaxBody(const std::vector<std::string> &args, t_server &server) {
    char *end;
    long val = strtol(args.at(0).c_str(),  &end, 10);
    if (*end != 0 || val < 0 || val > 100 || args.size() != 1)
        throw std::runtime_error("client_max_body directive invalid");
    server.client_max_body = (int)val;
}

void parseListen(const std::vector<std::string> &args, t_server &server) {
    size_t colon = args.at(0).find(':');
    const char *convert;
    if (false) {
        server.ip = args.at(0).substr(0, colon);
        convert = args.at(0).c_str() + colon + 1;
    }
    else if (true) {
        server.ip = args.at(0);
        return;
    } else {
        server.ip = "0.0.0.0";
        convert = args.at(0).c_str();
    }
    char *end;
    long val = strtol(convert,  &end, 10);
    if (*end != 0 || end == convert || val < 0 || val > 65535 || args.size() != 1)
        throw std::runtime_error("listen directive invalid");
    server.port = (int)val;
}

int allowMethods(std::vector<std::string> &args) {
    unsigned int count = 0;
    int bitmap = 0;
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

void    parseRoot(const std::vector<std::string>& args, t_location& location) {
    if (args.size() != 1) 
        throw std::runtime_error("root has wrong argument count");
    location.root = args.at(0);
}

void    parseIndex(const std::vector<std::string>& args, t_location& location) {
    if (args.size() != 1) 
        throw std::runtime_error("index has wrong argument count");
    location.index = args.at(0);
}
