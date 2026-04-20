#include "Parser.hpp"
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

void setDefault(t_eval &config) {
    config.clientsPerServer = 1024;
    config.maxClients = 1024;
}

void parser(t_eval& config, const char *fileName) {
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
        std::cout << "Error\n" << e.what() << "\n";
    }
}