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

Node *context(Tokenizer& stream, std::vector<std::string>& args);
Node *directive(std::vector<std::string>& args);
    
template<typename T>

int validateName(const std::string& specifier, const T& table) {
    int iter = 0;
    while (iter < table.size() && specifier.compare(table.at(iter).first)) //replace table.size with DIRECTIVECOUNT
        ++iter;
    if (iter == table.size())
        return -1;
    return iter;
}

template<typename T>
int    validate(const std::vector<std::string>& args, const T& table, const char* errormsg) {
    if (args.size() < 2)
        throw std::runtime_error(errormsg);
    int spec;
    spec = validateName(args.at(0), d);
    if (spec == -1)
        throw std::runtime_error(args.at(0) + " -> unknown");
    return spec;
}

void    fillDirective(Node *node, const std::vector<std::string>& args, const int spec) {
    node->tag = d.at(spec).second;
    node->type = DIRECTIVE;
    for (unsigned int i = 1;i < args.size(); ++i)
        node->args.push_back(args.at(i));
}


void    fillContext(Node *node, const std::vector<std::string>& args, const int spec) {
    node->tag = c.at(spec).second;
    node->type = CONTEXT;
    for (unsigned int i = 1;i < args.size(); ++i)
        node->args.push_back(args.at(i));
}

void    fillContent(Node *node, Tokenizer&stream, int type) {
    std::string token = stream.next();
    std::vector<std::string> specifier;
    while ((token.at(0) != '}' && type == CONTEXT) || (token.at(0) != EOF && type == BASE)) {
        switch (token.at(0)) {
            case '{':   node->content.push_back(context(stream, specifier));
            specifier.clear();
            break;
            case ';':   node->content.push_back(directive(specifier));
            specifier.clear();
            break;
            case EOF:   throw std::runtime_error("bracket not closed");
            default:    specifier.push_back(token);
        }
        token = stream.next();
    }
    if (!specifier.empty())
        throw std::runtime_error("something unfinished in context");
}

Node *directive(std::vector<std::string>& args) {
    int spec = validate(args, d, "empty directive");
    Node *directiveNode = new Node(); 
    fillDirective(directiveNode, args, spec);
    return directiveNode;
}

Node *context(Tokenizer& stream, std::vector<std::string>& args) {
    int spec = 0;
    validate(args, spec, "brackets opened without context");
    Node *contextNode = new Node();
    fillContext(contextNode, args, spec);
    fillContent(contextNode, stream, CONTEXT);
    return contextNode;
}

Node *base(Tokenizer& stream) {
    Node *base = new Node();
    base->tag = MAIN;
    base->type = CONTEXT;
    std::string token = stream.next();
    std::vector<std::string> specifier;
    fillContent(base, stream, BASE);
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

const t_conf& parser(const char *fileName) {
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