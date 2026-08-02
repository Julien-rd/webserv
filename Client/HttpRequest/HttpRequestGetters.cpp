#include "HttpRequest.hpp"

#include <string>

int                                       HttpRequest::getStatusCode() const { return _statusCode; }
size_t                                    HttpRequest::getBytesRead() const { return _bytesRead; }
const std::string                        &HttpRequest::getMethod() const { return _method; }
const std::string                        &HttpRequest::getUri() const { return _uri; }
const std::map<std::string, std::string> &HttpRequest::getHeaders() const { return _headers; }
state              HttpRequest::getCurrentState() const { return _currentState; }
size_t             HttpRequest::getContentLength() const { return _contentLength; }
const t_uri       &HttpRequest::getUriData() const { return _uriData; }
const std::string &HttpRequest::getBuffer() const { return _buffer; }
chunkedBodyState   HttpRequest::getChunkedBodyState() const { return _chunkedBodyState; }
size_t             HttpRequest::getBytesNeeded() const { return _bytesNeeded; }
std::string        HttpRequest::getHttpVersion() const { return _httpVersion; }
