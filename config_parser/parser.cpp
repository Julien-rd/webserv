#include "structs.hpp"
#include "Tokenizer.hpp"
#include <algorithm>
#include <cstddef>
#include <cstdio>
#include <exception>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

Node *directive(Tokenizer& stream, std::vector<std::string>& args) {
    Node *directiveNode = new Node(); 
    if (args.size() != 2)
        throw std::runtime_error("directive doesn't have exactly one argument");
    int spec = 0;
    while (spec < d.size() && args.at(0).compare(d.at(spec).first))
        ++spec;
    if (spec == d.size())
        throw std::runtime_error(args.at(0) + " -> directive unknown");
    directiveNode->tag = d.at(spec).second;
    directiveNode->type = DIRECTIVE;
    for (unsigned int i = 1;i < args.size(); ++i)
        directiveNode->args.push_back(args.at(i));
    return directiveNode;
}

Node *context(Tokenizer& stream, std::vector<std::string>& args) {
    Node *contextNode = new Node();
    if (args.empty())
        throw std::runtime_error("brackets opened without context");
    int spec = 0;
    while (spec < c.size() && args[0] != c[spec].first)
        ++spec;
    if (spec == c.size())
        throw std::runtime_error("context unknown");
    contextNode->tag = c[spec].second;
    contextNode->type = CONTEXT;
    for (unsigned int i = 1;i < args.size(); ++i)
        contextNode->args.push_back(args.at(i));
    std::string token = stream.next();
    std::vector<std::string> specifier;
    while (token.at(0) != '}') {
        switch (token.at(0)) {
            case '{':   contextNode->content.push_back(context(stream, specifier));
                        specifier.clear();
                        break;
            case ';':   contextNode->content.push_back(directive(stream, specifier));
                        specifier.clear();
                        break;
            case EOF:   throw std::runtime_error("bracket not closed");
            default:    specifier.push_back(token);
        }
        token = stream.next();
    }
    if (!specifier.empty())
        throw std::runtime_error("something unfinished in context");
    return contextNode;
}

Node *base(Tokenizer& stream) {
    Node *base = new Node();
    base->tag = MAIN;
    base->type = CONTEXT;
    std::string token = stream.next();
    std::vector<std::string> specifier;
    while (token.at(0) != EOF) {
        if (token.at(0) == '{') {
            base->content.push_back(context(stream, specifier));
            specifier.clear();
        }
        else if (token.at(0) == ';') {
            base->content.push_back(directive(stream, specifier));
            specifier.clear();
        }
        else
            specifier.push_back(token);
        token = stream.next();
    }
    return base;
}

# define GET 0
# define POST 1
# define DELETE 2

int allowMethods(std::vector<std::string> &args) {
    int bitmap = 0;
    if (std::find(args.begin(), args.end(), "GET") != args.end())
        bitmap |= 1 << GET;
    if (std::find(args.begin(), args.end(), "POST") != args.end())
        bitmap |= 1 << POST;
    if (std::find(args.begin(), args.end(), "DELETE") != args.end())
        bitmap |= 1 << DELETE;
    return bitmap;
}

void eval(Node *tree, t_eval &evalData) {
    if (tree->type == DIRECTIVE) {
        if (!evalData.server)
            throw std::runtime_error("directive outside of server block");
        switch (tree->tag) {
            case ROOT: evalData.servers.back().locations.back().root = tree->args[0]; return;
            case INDEX: evalData.servers.back().locations.back().index = tree->args[0]; return;
            case AUTOINDEX: evalData.servers.back().locations.back().autoindex = tree->args[0]; return;
            case ALLOWMETHODS: evalData.servers.back().locations.back().allowMethods = allowMethods(tree->args); return;
            default :   ;
        }
        if (evalData.location)
            throw std::runtime_error("wrong directive in location context");
        switch (tree->tag) {
            case SERVER_NAME: evalData.servers.back().server_name = tree->args[0]; break;
            case CLIENT_MAX_BODY_SIZE: evalData.servers.back().client_max_body = tree->args[0]; break;
            case PORT: evalData.servers.back().port = tree->args[0]; break;
            case LISTEN: evalData.servers.back().listen = tree->args[0]; break;
            default : throw std::runtime_error("directive unknown");
        }
    }
    else if (tree->type == CONTEXT) {
        if (tree->tag == SERVER) {
            if (evalData.server || evalData.location || !tree->args.empty())
                throw std::runtime_error("bad server block");
            evalData.server = true;
            t_server server;
            evalData.servers.push_back(server);
            t_location rootLocation{};
            rootLocation.location = "/";
            evalData.servers.back().locations.push_back(rootLocation);
        }
        else if (tree->tag == LOCATION) {
            if (!evalData.server || evalData.location || tree->args.size() != 1) 
                throw std::runtime_error("bad location block");
            evalData.location = true;
            t_location  newLocation{};
            newLocation.location = tree->args[0];
            evalData.servers.back().locations.push_back(newLocation);
        }
        for(unsigned int i = 0; i < tree->content.size(); ++i) {
            eval(tree->content.at(i), evalData);
        }
        if (tree->tag == SERVER)
            evalData.server = false;
        if (tree->tag == LOCATION)
            evalData.location = false;
    }
}

void freeTree(Node *node) {
    for (unsigned int i = 0; i < node->content.size(); ++i)
        freeTree(node->content[i]);
    delete node;
}

void parser(const char *fileName) {
    
    Node *tree = NULL;
    try {
        Tokenizer stream(fileName);
        tree = base(stream);
        t_eval  evalData{};
        eval(tree, evalData);
        if (tree)
            freeTree(tree);
    } catch (std::exception &e) {
        if (tree)
            freeTree(tree);
        std::cout << "Error\n" << e.what() << "\n";
    } 
}

int main() {
    parser("test.txt");
}