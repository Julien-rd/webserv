#include "HttpRequest.hpp"
#include <iostream>
#include <sstream>
#include <string>

HttpRequest::HttpRequest(size_t client_max_body_size)
    : _currentState(METHOD), _contentLength(0), _parsingDone(false),
      _client_max_body_size(client_max_body_size) {}

HttpRequest::HttpRequest()
    : _currentState(METHOD), _contentLength(0), _parsingDone(false),
      _client_max_body_size(300000) {} // TODO hardcoded fix accordingly

std::vector<char> HttpRequest::getBody() const { return _body; }

HttpRequest::HttpRequest(const HttpRequest& obj) {
  _method = obj._method;
  _uri = obj._uri;
  _headers = obj._headers;
  _currentState = obj._currentState;
  _contentLength = obj._contentLength;
  _uriData = obj._uriData;
  _httpVersion = obj._httpVersion;
  _fieldName = obj._fieldName;
  _fieldValue = obj._fieldName;
  _bytesRead = obj._bytesRead;
  _statusCode = obj._statusCode;
  _parsingDone = obj._parsingDone;
  _body = obj._body;
  _client_max_body_size = obj._client_max_body_size;
}

const HttpRequest& HttpRequest::operator=(const HttpRequest& obj) {
  if (&obj == this) {
    return *this;
  }
  _method = obj._method;
  _uri = obj._uri;
  _headers = obj._headers;
  _currentState = obj._currentState;
  _contentLength = obj._contentLength;
  _uriData = obj._uriData;
  _httpVersion = obj._httpVersion;
  _fieldName = obj._fieldName;
  _fieldValue = obj._fieldName;
  _bytesRead = obj._bytesRead;
  _statusCode = obj._statusCode;
  _parsingDone = obj._parsingDone;
  _body = obj._body;
  _client_max_body_size = obj._client_max_body_size;
  return *this;
}

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
  else {
    _statusCode = 400;
    return 1;
  }
}

#include <cstdlib>
int HttpRequest::parseRequestLine(std::string& request_content) {
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
    /* fall through */
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
    /* fall through */
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
    /* fall through */
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

bool HttpRequest::containsWhiteSpaces() {
  size_t pos;
  pos = _fieldName.find_first_of(" \t\r\n");
  if (pos != std::string::npos) {
    _statusCode = 400;
    return true;
  }
  pos = _fieldName.find_last_of(" \t\r\n");
  if (pos != std::string::npos) {
    _statusCode = 400;
    return true;
  }
  return false;
}

int HttpRequest::parseHeaders(std::string& request_content) {
  size_t pos;
  size_t max_pos;
  while (_bytesRead < request_content.length()) {
    switch (_currentState) {
    case FIELD_NAME:
      findSeperator(request_content, ':', pos, max_pos);
      if (pos > max_pos) {
        // TODO add safguard to check if it is really last line so \r\n
        _bytesRead += 1;
        _currentState = EOH;
        break;
      }
      if (pos == std::string::npos) {
        _fieldName += request_content.substr(_bytesRead);
        return 0;
      }
      exctractContent(request_content, pos);
      if (containsWhiteSpaces() == true) {
        // print();
        return 1;
      }
      _currentState = FIELD_VALUE;
      /* fall through */
    case FIELD_VALUE:
      pos = request_content.find("\r", _bytesRead);
      if (pos == std::string::npos) {
        _fieldValue += request_content.substr(_bytesRead);
        // FIX: claude said it needs this here: _bytesRead = request_content.size();
        return 0;
      }
      exctractContent(request_content, pos);
      trim();
      addHeader();
      _currentState = CR;
      /* fall through */
    case CR:
      pos = request_content.find("\n", _bytesRead);
      if (_bytesRead >= request_content.size())
        return 0;
      if (validNewLine(request_content) == 1)
        return 1;
      exctractContent(request_content, pos);
      _currentState = FIELD_NAME;
      break;
    case EOH: // TODO what if \r \n are sent seperatly
      if (_bytesRead >= request_content.size())
        return 0;
      if (validNewLine(request_content) == 1)
        return 1;
      ++_bytesRead;
      _currentState = BODY;
      /* fall through */
    default:
      return 0;
    }
  }
  return 0;
}

int HttpRequest::parse_body(std::string request_content) {
  if (_currentState == BODY) {
    size_t                bytes_needed = _contentLength - _body.size();
    std::string::iterator start = request_content.begin() + _bytesRead;
    std::string::iterator end = request_content.end();
    if (start + bytes_needed > end) {
      std::cout << "BIG L\n";
      _body.insert(_body.end(), start, end);
    } else {
      std::cout << "BIG W\n";
      _body.insert(_body.end(), start, start + bytes_needed);
      std::cout << std::endl;
      _parsingDone = true;
      _statusCode = 200;
    }
    _bytesRead += end - start;
    std::cout << "bytesRead: " << _bytesRead << "\n";
    std::cout << "end - start : " << end - start << "\n";
    std::cout << "contentLength: " << _contentLength << "\n";
    std::cout << "bogySize: " << _body.size() << "\n";
  }
  if (_currentState == BODY_CHUNKED) {
    // implement code for chunked_uri
  }
  return 0;
}

int HttpRequest::parseHttpRequest(std::string request_content,
                                  size_t      bytes_read) {
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
    if (_currentState != BODY) {
      _parsingDone = true;
      _statusCode = 200;
      return 0;
    }
    if (parse_body(request_content) == 1) {
      std::cout << "body issue\n";
      return 1;
    }
  }
  return 0;
}

std::string percentDecode(const std::string& encoded, bool isQuery) {
    std::string result;
    for (size_t i = 0; i < encoded.size(); ++i) {
        if (encoded[i] == '%' && i + 2 < encoded.size()) {
            int                val;
            std::istringstream hex(encoded.substr(i + 1, 2));
            hex >> std::hex >> val;
            result += static_cast<char>(val);
            i += 2;
        } else if (isQuery && encoded[i] == '+') {
            result += ' '; // only in query strings, not paths
        } else {

            result += encoded[i];
        }
    }
    return result;
}

int HttpRequest::parseURIContent(void) {
    size_t      qmark = _uri.find('?');
    std::string path = _uri.substr(0, qmark);
    path = percentDecode(path, false);
    if (validateURIPath(path) == false) {
        std::cerr << "invalid path in URI" << std::endl;
        return 1;
    }
    _uriData.query = (qmark != std::string::npos) ? _uri.substr(qmark + 1) : "";

    size_t dot = path.rfind('.');
    size_t lastSlash = path.rfind('/');
    if (lastSlash == 0) {
        path.append("/"); // Fix: when does this make sense? /index.html will be
        // /index.html/ for what reason
        lastSlash = path.size() - 1;
    }
    if (dot != std::string::npos && dot < lastSlash) {
        size_t extEnd = path.find('/', dot);
        _uriData.extension = path.substr(dot, extEnd - dot);
        _uriData.path = path.substr(0, extEnd);
        _uriData.pathInfo =
            (extEnd != std::string::npos) ? path.substr(extEnd) : "";
    } else {
        if (dot != std::string::npos)
            _uriData.extension = path.substr(dot);
        _uriData.path = path;
    }
    _uriData.query = percentDecode(
        _uriData.query, true); // Fix: uriData.query passed and received
    _uri = _uriData.path;
    // std::cout << "from URI: " << _uri << " parsed =>\n"
    //           << "path:   " << _uriData.path << "\next:  " <<
    //           _uriData.extension
    //           << "\npathInfo: " << _uriData.pathInfo
    //           << "\nquery: " << _uriData.query << "\n";
    return 0;
}
