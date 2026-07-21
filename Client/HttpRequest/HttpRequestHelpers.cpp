#include "HttpRequest.hpp"
#include <cctype>
#include <iostream>
#include <sstream>
#include <string>

void HttpRequest::addHeader() {
  std::string data = "Abc";
  std::string fieldNameToLow = _fieldName;
  for (size_t it = 0; it < _fieldName.size(); ++it)
    fieldNameToLow[it] = tolower(_fieldName[it]);
  if (_headers.count(fieldNameToLow) > 0) {
    _headers[fieldNameToLow] += ", " + _fieldValue;
  } else
    _headers[fieldNameToLow] = _fieldValue;
  _fieldName.clear();
  _fieldValue.clear();
}

void HttpRequest::trim() {
  size_t pos;
  pos = _fieldValue.find_first_not_of(" \t\r\n");
  if (pos != std::string::npos)
    _fieldValue.erase(0, pos);
  pos = _fieldValue.find_last_not_of(" \t\r\n");
  if (pos != std::string::npos)
    _fieldValue.erase(pos + 1);
}

void HttpRequest::exctractContent(std::string& recvBuffer, size_t pos) {
  size_t skip = 1;
  switch (_currentState) {
  case METHOD:
    _method += recvBuffer.substr(_bytesRead, pos - _bytesRead);
    break;
  case URI:
    _uri += recvBuffer.substr(_bytesRead, pos - _bytesRead);
    break;
  case HTTP_VERSION:
    _httpVersion += recvBuffer.substr(_bytesRead, pos - _bytesRead);
    // skip = 2;
    break;
  case FIELD_NAME:
    _fieldName += recvBuffer.substr(_bytesRead, pos - _bytesRead);
    break;
  case FIELD_VALUE:
    _fieldValue += recvBuffer.substr(_bytesRead, pos - _bytesRead);
    // skip = 2;
    break;
  case CR:
    skip = 1;
  default:;
  }
  _bytesRead = pos + skip;
}

bool HttpRequest::brokenSyntax(size_t pos, size_t max_pos) {
  if (pos == std::string::npos && max_pos != std::string::npos) {
    _statusCode = 400;
    return 1;
  }
  return 0;
}

void HttpRequest::findSeperator(std::string& recvBuffer, char seperator,
                                size_t& pos, size_t& max_pos) {
  max_pos = recvBuffer.find("\r", _bytesRead);
  pos = recvBuffer.find(seperator, _bytesRead);
}

bool HttpRequest::validMethod() {
  if (!(_method == "GET" || _method == "POST" || _method == "DELETE")) {
    std::cout << "invalid Method\n";
    _statusCode = 405; // 501 for not implemented, but existing method
    return false;
  }
  return true;
}

bool HttpRequest::validUri() {
  if (_uri.size() > 4096) {
    _statusCode = 414;
    std::cout << "invalid URI\n";
    return false;
  }
  if (_uri.find("#") != std::string::npos) {
    _statusCode = 400;
    std::cout << "invalid URI\n";
    return false;
  }
  if (_uri.find("//") != std::string::npos) {
    _statusCode = 400;
    std::cout << "invalid URI\n";
    return false;
  }
  if (_uri.find('\0') != std::string::npos) {
    _statusCode = 400;
    std::cout << "invalid URI\n";
    return false;
  }
  return (true);
}

bool HttpRequest::validateURIPath(std::string& path) {
  if (*(path.begin()) != '/') {
    _statusCode = 400;
    std::cout << "ERROR: path doesn't begin with '/'\n";
    return false;
  }
  for (std::string::iterator it = path.begin(); it != path.end(); ++it) {
    if (*it < 33 || *it > 126) {
      _statusCode = 400;
      std::cout << "invalid URI\n";
      return false;
    }
  }
  if (path.size() > 2 && (path.find("/../") != std::string::npos ||
      path.rfind("/..") == path.size() - 3)) {
    _statusCode = 400;
    std::cout << "ERROR: escape root sequence found in URI path\n";
    return false;
  }
  return true;
}

#include <csignal>
#include <cstdlib>
bool HttpRequest::validHttpsVersion() {
  if (_httpVersion !=
      "HTTP/1.1") { // TODO does HTTP/1.1 need to be backward compatible? in
                    // that case maybe we can't make this check
    _statusCode = 400;
    std::cout << "HTTP version\n";
    return false;
  }
  return true;
}

bool HttpRequest::hasHostHeader() {
  std::map<std::string, std::string>::iterator it = _headers.find("host");
  if (it == _headers.end())
    return false;
  return true;
}

bool HttpRequest::hasContentLength() {
  std::map<std::string, std::string>::iterator it =
      _headers.find("content-length");
  if (it != _headers.end()) {
    if (_currentState == BODY_CHUNKED) {
      std::cerr << "Header has both transfer-encoding and content-length\n";
      return false;
    }
    std::stringstream ss(_headers["content-length"]);
    ss >> _contentLength;
    if (ss.fail()) {
      std::cerr << "Invalid Content-Length\n";
      return false;
    }
    if (_contentLength > _client_max_body_size) {
      std::cerr << "Request Entity Too Large\n<";
      return false;
    }
  }
  return true;
}

void HttpRequest::isChunked() {
  std::map<std::string, std::string>::iterator it =
      _headers.find("transfer-encoding");
  if (it == _headers.end())
    return;
  if (it->second == "chunked")
    _currentState = BODY_CHUNKED;
  return;
}

bool HttpRequest::validateMandatoryHeaders() {
  if (_method == "POST") {
    isChunked();
    return (hasHostHeader() && hasContentLength());
  }
  _currentState = METHOD;
  return (hasHostHeader());
}

void HttpRequest::reset() {
  _currentState = METHOD;
  _method.clear();
  _uri.clear();
  _httpVersion.clear();
  _fieldName.clear();
  _fieldValue.clear();
  _contentLength = 0;
  // _bytesRead = 0;
  _statusCode = 0;
  _parsingDone = false;
  _headers.clear();
  _body.clear();
}
