#include "Node.hpp"
#include "structs.hpp"
#include "Tokenizer.hpp"
#include <cstdio>
#include <iostream>
#include <stdexcept>
#include <string>

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

void eval(Node *tree, t_eval &evalData) {
    if (tree->type == DIRECTIVE) {
        if (!evalData.server)
            throw std::runtime_error("directive outside of server block");
        switch (tree->tag) {
            case ROOT: evalData.servers.back().root = tree->args[0]; return;
            case INDEX: evalData.servers.back().index = tree->args[0]; return;
            default :   ;
        }
        if (evalData.location)
            throw std::runtime_error("wrong directive in location context");
        switch (tree->tag) {
            case SERVER_NAME: evalData.servers.back().server_name = tree->args[0]; break;
            case LISTEN: evalData.servers.back().listen = tree->args[0]; break;
            case CLIENT_MAX_BODY_SIZE: evalData.servers.back().client_max_body = tree->args[0]; break;
            case PORT: evalData.servers.back().port = tree->args[0]; break;
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
        }
        else if (tree->tag == LOCATION) {
            if (!evalData.server || evalData.location || tree->args.size() != 1) 
                throw std::runtime_error("bad location block");
            evalData.location = true;
            evalData.servers.back().location = tree->args[0];
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

void parser(const char *fileName) {
    Tokenizer stream(fileName);
    Node *tree = base(stream);
    
    t_eval  evalData{};
    eval(tree, evalData);
    for (unsigned int i = 0; i < evalData.servers.size(); ++i) {
        std::cout << "\nServer nr: " << i + 1 << "\n";
        std::cout << "server name: " << evalData.servers[i].server_name << "\n";
        std::cout << "client_max_body: " << evalData.servers[i].client_max_body << "\n";
        std::cout << "index: " << evalData.servers[i].index << "\n";
        std::cout << "listen: " << evalData.servers[i].listen << "\n";
        std::cout << "port: " << evalData.servers[i].port << "\n";
        std::cout << "root: " << evalData.servers[i].root << "\n";
    }
}

int main() {
    parser("test.txt");
}