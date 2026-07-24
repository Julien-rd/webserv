#include <cstdio>
#include <fstream>

class Tokenizer {
  public:
    Tokenizer(const char *fileName);
    ~Tokenizer();

    const std::string next();

  private:
    std::ifstream _file;
    void          skipWhitespaces();
};
