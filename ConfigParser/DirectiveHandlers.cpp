#include "../Utils/Macros.hpp"
#include "Parser.hpp"
#include "Structs.hpp"

#include <algorithm>
#include <vector>

void parseLogLvl(const std::vector<std::string> &args, Level::Value &logLvl) {
    if (args.size() != 1)
        throw std::runtime_error("log_level directive invalid");

    const std::string &value = args.at(0);

    if (value == "default")
        logLvl = Level::DEFAULT;
    else if (value == "info")
        logLvl = Level::INFO;
    else if (value == "debug")
        logLvl = Level::DEBUG;
    else if (value == "error")
        logLvl = Level::ERROR;
    else if (value == "warning")
        logLvl = Level::WARNING;
    else
        throw std::runtime_error("log_level has wrong argument");
}

void parseAutoindex(const std::vector<std::string> &args, t_location &location) {
    if (args.size() != 1)
        throw std::runtime_error("autoindex directive invalid");
    if (args.at(0) == "on")
        location.autoindex = true;
    else if (args.at(0) == "off")
        location.autoindex = false;
    else
        throw std::runtime_error("autoindex directive invalid");
}

void parseLocalhost(const std::vector<std::string> &args, t_server &server) {
    if (args.size() != 1)
        throw std::runtime_error("localhost directive invalid");
    if (args[0] == "yes")
        server.localhost = true;
}

void parseNumberGlobal(const std::vector<std::string> &args, unsigned int *number) {
    char *end;
    long  val = strtol(args.at(0).c_str(), &end, 10);
    if (*end != 0 || val < 0 || val > 5000 || args.size() != 1)
        throw std::runtime_error("Global directive invalid");
    *number = (int) val;
}

void parseMaxBody(const std::vector<std::string> &args, t_server &server) {
    char *end;
    long  val = strtol(args.at(0).c_str(), &end, 10);
    if (*end != 0 || val < 0 || val > 100 || args.size() != 1)
        throw std::runtime_error("client_max_body directive invalid");
    server.clientMaxBody = (unsigned int) val * MEGABYTE;
}

void parseListen(const std::vector<std::string> &args, t_server &server) {
    if (args.size() != 1) {
        throw std::runtime_error("listen directive invalid");
    }
    size_t      colon = args.at(0).find(':');
    const char *convert;
    if (colon != std::string::npos) {  // "ip:port" form
        server.ip = args.at(0).substr(0, colon);
        convert = args.at(0).c_str() + colon + 1;
    } else if (args.at(0).find_first_not_of("0123456789") == std::string::npos) {  // "port" form
        server.ip = "0.0.0.0";
        convert = args.at(0).c_str();
    } else {  // "ip" form
        server.ip = args.at(0);
    }
    char *end;
    long  val = strtol(convert, &end, 10);
    if (*end != 0 || end == convert || val < 0 || val > 65535)
        throw std::runtime_error("listen directive invalid");
    server.port = std::string(convert);
}

void allowMethods(const std::vector<std::string> &args, t_location &location) {
    if (args.size() == 0 || args.size() > 3)
        throw std::runtime_error("allow_methods has wrong number of arguments");
    for (unsigned int i = 0; i < args.size(); ++i) {
        if ((args.at(i) != "GET" && args.at(i) != "POST" && args.at(i) != "DELETE") ||
            std::find(location.allowMethods.begin(), location.allowMethods.end(), args.at(i)) !=
                location.allowMethods.end())
            throw std::runtime_error("allow_methods has wrong arguments");
        location.allowMethods.push_back(args.at(i));
    }
}

void parseRoot(const std::vector<std::string> &args, t_location &location) {
    if (args.size() != 1)
        throw std::runtime_error("root has wrong argument count");
    location.root = args.at(0);
}

void parseTryFiles(const std::vector<std::string> &args, t_location &location) {
    if (args.size() > 4)
        throw std::runtime_error("try_files has too many arguments");
    for (unsigned int i = 0; i < args.size(); ++i)
        location.tryFiles.push_back(args.at(i));
}

void parseIndex(const std::vector<std::string> &args, t_location &location) {
    if (args.size() != 1)
        throw std::runtime_error("index has wrong argument count");
    location.index = args.at(0);
}

void parseAlias(const std::vector<std::string> &args, t_location &location) {
    if (args.size() != 1)
        throw std::runtime_error("alias has wrong argument count");
    location.alias = args.at(0);
}

void parseReturn(const std::vector<std::string> &args, t_location &location) {
    if (args.size() != 2)
        throw std::runtime_error("return has wrong argument count");
    char *end;
    long  val = strtol(args.at(0).c_str(), &end, 10);
    if (*end != 0 || end == args.at(0).c_str() || val < 0 || val > 500)
        throw std::runtime_error("return directive invalid");
    location.redirect.first = (int) val;
    location.redirect.second = args.at(1);
}

void parseCGIConfigs(const std::vector<std::string> &args, t_server &server) {
    if (args.size() < 2) {
        throw std::runtime_error("cgi_config needs more arguments");
    }
    for (size_t i = 0; i < server.cgiConfigs.size(); i++) {
        if (server.cgiConfigs.at(i).extension == args.at(0)) {
            throw std::runtime_error("duplicate cgi extensions");
        }
    }
    t_cgi_config cgiConfig;
    cgiConfig.extension = args.at(0);
    cgiConfig.executablePath = args.at(1);
    server.cgiConfigs.push_back(cgiConfig);
}
