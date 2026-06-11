#include "Tokenizer.hpp"

#include <stdexcept>
#include <string>

Tokenizer::Tokenizer(const char* fileName) {
  _file.open(fileName);
  if (!_file.is_open())
    throw std::runtime_error("file couldn't be opened");
}

Tokenizer::~Tokenizer() {
  if (_file.is_open())
    _file.close();
}

const std::string Tokenizer::next() {
  skipWhitespaces();
  char        peek = _file.peek();
  std::string token;
  if (peek == EOF || peek == '{' || peek == '}' || peek == ';')
    return token += _file.get();
  while (peek != ' ' && peek != '\n' && peek != EOF && peek != '{' &&
         peek != '}' && peek != ';') {
    token += _file.get();
    peek = _file.peek();
  }
  return token;
}

void Tokenizer::skipWhitespaces() {
  std::string whitespace = " \t\n\r";
  while (whitespace.find(_file.peek()) != std::string::npos &&
         _file.peek() != EOF)
    _file.get();
}
