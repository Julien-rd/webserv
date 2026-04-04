#include "HttpRequest.hpp"
#include <iostream>
#include <string>

HttpRequest::HttpRequest(size_t client_max_body_size)
    : _currentState(METHOD), _contentLength(0), _parsingDone(false),
      _client_max_body_size(client_max_body_size) {}

HttpRequest::HttpRequest()
    : _currentState(METHOD), _contentLength(0), _parsingDone(false),
      _client_max_body_size(1) {}

void HttpRequest::print() {
  std::cout << "---------------------RequestLine---------------------\n";
  std::cout << "Method: [" << _method << "]\n"
            << "URI: [" << _uri << "]\n"
            << "HTTP_VERSION: [" << _httpVersion << "]\n";
  std::cout << "\n---------------------Headers---------------------\n";
  for (std::map<std::string, std::string>::iterator it = _headers.begin();
       it != _headers.end(); ++it) {
    std::cout << "Fieldname: [" << it->first << "], Fieldvalue: [" << it->second
              << "]" << "\n";
  }
}
bool HttpRequest::parsingDone() { return _parsingDone; }

bool HttpRequest::validNewLine(std::string request_content) {
  if (!request_content.empty() && request_content[_bytesRead] == '\n') 
    return 0;
  else{
    _statusCode = 400;
    return 1;
  }
}

#include <cstdlib>
int HttpRequest::parseRequestLine(std::string &request_content) {
  size_t pos;
  size_t max_pos;
  switch (_currentState) {
  case METHOD:
    findSeperator(request_content, ' ', pos, max_pos);
    if (brokenSyntax(pos, max_pos))
      return 1;
    if (pos == std::string::npos) {
      _method += request_content.substr(_bytesRead);
      break;
    }
    exctractContent(request_content, pos);
    if (validMethod() == false)
      return 1;
    _currentState = URI;
  case URI:
    findSeperator(request_content, ' ', pos, max_pos);
    if (brokenSyntax(pos, max_pos))
      return 1;
    if (pos == std::string::npos) {
      _uri += request_content.substr(_bytesRead);
      break;
    }
    exctractContent(request_content, pos);
    if (validUri() == false)
      return 1;
    _currentState = HTTP_VERSION;
  case HTTP_VERSION:
    pos = request_content.find("\r", _bytesRead);
    if (pos == std::string::npos) {
      _httpVersion += request_content.substr(_bytesRead);
      break;
    }
    exctractContent(request_content, pos);
    if (validHttpsVersion() == false)
      return 1;
    _currentState = CR;
  case CR:
    pos = request_content.find("\n", _bytesRead);
    if (_bytesRead >= request_content.size())
      return 0;
    if (validNewLine(request_content) == 1) 
      return 1;
    exctractContent(request_content, pos);
    _currentState = FIELD_NAME;
  default:;
  }
  return 0;
}

bool HttpRequest::containsWhiteSpaces(){
  size_t pos;
  pos = _fieldName.find_first_of(" \t\r\n");
  if (pos != std::string::npos){
    _statusCode = 400;
    return true;
  }
  pos = _fieldName.find_last_of(" \t\r\n");
  if (pos != std::string::npos){
    _statusCode = 400;
    return true;
  }
  return false;
}

int HttpRequest::parseHeaders(std::string &request_content) {
  size_t pos;
  size_t max_pos;
  while (_bytesRead < request_content.length()) {
    switch (_currentState) {
    case FIELD_NAME:
      findSeperator(request_content, ':', pos, max_pos);
      if (pos == std::string::npos && max_pos != std::string::npos) {
        // add safguard to check if it is really last line so \r\n
        _bytesRead += 1;
        _currentState = EOH;
        break ;
      }
      if (pos == std::string::npos) {
        _fieldName += request_content.substr(_bytesRead);
        return 0;
      }
      exctractContent(request_content, pos);
      if(containsWhiteSpaces() == true){
        std::cout << "[" << request_content << "]\n";
        std::cout << "FUCKK\n";
        return 1;
      }
      _currentState = FIELD_VALUE;
    case FIELD_VALUE:
      pos = request_content.find("\r", _bytesRead);
      if (pos == std::string::npos) {
        _fieldValue += request_content.substr(_bytesRead);
        return 0;
      }
      exctractContent(request_content, pos);
      trim();
      addHeader();
      _currentState = CR;
    case CR:
      pos = request_content.find("\n", _bytesRead);
      if (_bytesRead >= request_content.size())
        return 0;
      if (validNewLine(request_content) == 1){
        std::cout << "FUCKKfd\n";
        return 1;
      }
      exctractContent(request_content, pos);
      _currentState = FIELD_NAME;
      break ;
    case EOH:
      if (_bytesRead >= request_content.size())
        return 0;
      if (validNewLine(request_content) == 1){
        std::cout << "FUCKKda\n";
        return 1;
      }
      ++_bytesRead;
      _currentState = BODY;
    default:
      return 0;
    }
  }
  return 0;
}

int HttpRequest::parse_body(std::string request_content) {
  if (_currentState == BODY) {
    size_t bytes_needed = _contentLength - _body.size();
    std::string::iterator start = request_content.begin();
    std::string::iterator end = request_content.end();
    if (start + bytes_needed > end)
      _body.insert(_body.end(), start, end);
    else
      _body.insert(_body.end(), start, start + bytes_needed);
  }
  if (_currentState == BODY_CHUNKED) {
    // implement code for chunked
  }
  return 0;
}

int HttpRequest::parseHttpRequest(std::string request_content,
                                  size_t bytes_read) {
  _bytesRead = bytes_read;
  _parsingDone = false;
  if (parseRequestLine(request_content) == 1) {
    std::cout << "RequestLine Issue\n";
    return 1;
  }
  if (parseHeaders(request_content) == 1) {
    std::cout << "Header Issue\n";
    return 1;
  }
  if (_currentState == BODY) {
    if (validateMandatoryHeaders() == false) {
      std::cout << "validation of mandatory header issue\n";
      return 1;
    }
    if (parse_body(request_content) == 1) {
      std::cout << "body issue\n";
      return 1;
    }
    _parsingDone = true;
    _statusCode = 200;
  }
  return 0;
}

int HttpRequest::getStatusCode() const { return _statusCode; }

size_t HttpRequest::getBytesRead() const { return _bytesRead; }

