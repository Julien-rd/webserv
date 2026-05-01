#include <cstdio>
#include <fstream>
#include <stdexcept>

class Tokenizer {
public:
  Tokenizer(const char *fileName) {
    file.open(fileName);
    if (!file.is_open())
      throw std::runtime_error("file couldn't be opened");
  }
  ~Tokenizer() {
    if (file.is_open())
      file.close();
  }
  const std::string next() {
    skipWhitespaces();
    char peek = file.peek();
    std::string token;
    if (peek == EOF || peek == '{' || peek == '}' || peek == ';')
      return token += file.get();
    while (peek != ' ' && peek != '\n' && peek != EOF && peek != '{' &&
           peek != '}' && peek != ';') {
      token += file.get();
      peek = file.peek();
    }
    return token;
  }

private:
  std::ifstream file;
  void skipWhitespaces() {
    std::string whitespace = " \t\n\r";
    while (whitespace.find(file.peek()) != std::string::npos &&
           file.peek() != EOF)
      file.get();
  }
};
