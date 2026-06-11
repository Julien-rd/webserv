#include "HttpRequest.hpp"
#include <string>

std::string HttpRequest::getURI() const { return _uri; }
int HttpRequest::getStatusCode() const { return _statusCode; }
size_t HttpRequest::getBytesRead() const { return _bytesRead; }
std::string HttpRequest::getMethod() const { return _method; }
