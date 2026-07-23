#include "Parser.hpp"
#include "Structs.hpp"

#include <cstring>

void evalDirective(Node *tree, t_config &evalData) {
    if (!evalData.serverFlag) {
        switch (tree->tag) {
        case MAX_CLIENTS:
            parseNumberGlobal(tree->args, &evalData.maxClients);
            return;
        case CLIENT_TIMEOUT:
            parseNumberGlobal(tree->args, &evalData.clientTimeout);
            return;
        case CLIENTS_PER_SERVER:
            parseNumberGlobal(tree->args, &evalData.clientsPerServer);
            return;
        default:
            throw std::runtime_error("wrong directive outside of server block");
        }
    }
    t_location &currLocation = evalData.locationFlag ? evalData.servers.back().locations.back()
                                                     : evalData.servers.back().locations.front();
    switch (tree->tag) {
    case TRYFILES:
        parseTryFiles(tree->args, currLocation);
        return;
    case ROOT:
        parseRoot(tree->args, currLocation);
        return;
    case INDEX:
        parseIndex(tree->args, currLocation);
        return;
    case AUTOINDEX:
        parseAutoindex(tree->args, currLocation);
        return;
    case RETURN:
        parseReturn(tree->args, currLocation);
        return;
    case ALIAS:
        parseAlias(tree->args, currLocation);
        return;
    case ALLOWMETHODS:
        allowMethods(tree->args, currLocation);
        return;
    default:;
    }
    if (evalData.locationFlag)
        throw std::runtime_error("wrong directive in location context");
    switch (tree->tag) {
    // case SERVER_NAME: evalData.servers.back().server_name = tree->args[0];
    // break;
    case CLIENT_MAX_BODY_SIZE:
        parseMaxBody(tree->args, evalData.servers.back());
        break;
    case LISTEN:
        parseListen(tree->args, evalData.servers.back());
        break;
    case LOCALHOST:
        parseLocalhost(tree->args, evalData.servers.back());
        break;
    case CGI_CONFIG:
        parseCGIConfigs(tree->args, evalData.servers.back());
        return;
    default:
        throw std::runtime_error("directive unknown");
    }
}

void setServer(t_server &server) {
    server.clientMaxBody = 2;
    server.ip = "0.0.0.0";
    server.port = "8080";
    server.localhost = false;
}

void setLocation(t_location &location) { location.autoindex = false; }

void evalContext(Node *tree, t_config &evalData) {
    if (tree->tag == SERVER) {
        if (evalData.serverFlag || evalData.locationFlag || !tree->args.empty())
            throw std::runtime_error("bad server block");
        evalData.serverFlag = true;
        t_server server;
        setServer(server);
        evalData.servers.push_back(server);
        t_location rootLocation;
        setLocation(rootLocation);
        rootLocation.name = "/";
        evalData.servers.back().locations.push_back(rootLocation);
    } else if (tree->tag == LOCATION) {
        if (!evalData.serverFlag || evalData.locationFlag || tree->args.size() != 1)
            throw std::runtime_error("bad location block");
        evalData.locationFlag = true;
        t_location newLocation;
        setLocation(newLocation);
        newLocation.name = tree->args[0];
        evalData.servers.back().locations.push_back(newLocation);
    }
    for (unsigned int i = 0; i < tree->content.size(); ++i) {
        evalTree(tree->content.at(i), evalData);
    }
    if (tree->tag == SERVER)
        evalData.serverFlag = false;
    if (tree->tag == LOCATION)
        evalData.locationFlag = false;
}

void evalTree(Node *tree, t_config &evalData) {
    if (tree->type == DIRECTIVE)
        evalDirective(tree, evalData);
    else if (tree->type == CONTEXT)
        evalContext(tree, evalData);
}
