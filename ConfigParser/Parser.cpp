#include "Parser.hpp"
#include "../Logger/logger.hpp"

#include <cstddef>
#include <cstdio>
#include <exception>
#include <iostream>
#include <vector>

void freeTree(Node *node) {
    for (unsigned int i = 0; i < node->content.size(); ++i)
        freeTree(node->content[i]);
    delete node;
}

void setDefault(t_config &config) {
    config.serverFlag = false;
    config.locationFlag = false;
    config.clientsPerServer = 1024;
    config.maxClients = 1024;
    config.clientTimeout = 50;
    config.logLvl = Level::DEFAULT;
}

bool parseConfigFile(t_config &config, const char *fileName) {
    Node *tree = NULL;
    setDefault(config);
    try {
        Tokenizer stream(fileName);
        tree = parseTree(stream);
        evalTree(tree, config);
        if (tree)
            freeTree(tree);
    } catch (std::exception &e) {
        if (tree)
            freeTree(tree);
        std::cout << "Error: " << e.what() << "\n";
        return true;
    }
    return false;
}
