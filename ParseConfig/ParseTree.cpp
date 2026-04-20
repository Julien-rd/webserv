#include "Parser.hpp"

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
    if (args.empty())
        throw std::runtime_error(errormsg);
    int spec;
    spec = validateName(args.at(0), table);
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
    if (!specifier.empty() && specifier.at(0) == "}")
        throw std::runtime_error("closing bracket without open context");
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
    int spec = validate(args, c, "brackets opened without context");
    Node *contextNode = new Node();
    fillContext(contextNode, args, spec);
    fillContent(contextNode, stream, CONTEXT);
    return contextNode;
}

Node *parseTree(Tokenizer& stream) {
    Node *base = new Node();
    base->tag = MAIN;
    base->type = CONTEXT;
    fillContent(base, stream, BASE);
    return base;
}
