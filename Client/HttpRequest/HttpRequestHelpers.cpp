#include "../../Logger/Logger.hpp"
#include "../../Utils/Macros.hpp"
#include "HttpRequest.hpp"

#include <cctype>
#include <string>

bool HttpRequest::addHeader() {
    if (_headers.size() == MAX_HEADERS) {
        _statusCode = 431;
        return false;
    }
    size_t bytesToAdd = _fieldName.size() + _fieldValue.size();
    if (bytesToAdd > MAX_HEADER_SUM - _headerBytes) {
        _statusCode = 431;
        return false;
    }
    _headerBytes += bytesToAdd;
    std::string fieldNameToLow = _fieldName;
    for (size_t it = 0; it < _fieldName.size(); ++it)
        fieldNameToLow[it] = tolower(_fieldName[it]);
    if (_headers.count(fieldNameToLow) > 0) {
        _headers[fieldNameToLow] += ", " + _fieldValue;
    } else
        _headers[fieldNameToLow] = _fieldValue;
    _fieldName.clear();
    _fieldValue.clear();
    return true;
}

void HttpRequest::setStatusCode(int status) { _statusCode = status; }

void HttpRequest::trim() {
    size_t pos;
    pos = _fieldValue.find_first_not_of(" \t\r\n");
    if (pos != std::string::npos)
        _fieldValue.erase(0, pos);
    pos = _fieldValue.find_last_not_of(" \t\r\n");
    if (pos != std::string::npos)
        _fieldValue.erase(pos + 1);
}

bool HttpRequest::isTooLong(size_t pending) {
    switch (_currentState) {
    case METHOD:
        if (_method.size() >= MAX_METHOD_LEN || pending > MAX_METHOD_LEN - _method.size()) {
            _statusCode = 501;
            log(Level::WARNING, "isTooLong: METHOD");
            return true;
        }
        break;
    case URI:
        if (_uri.size() >= MAX_URI_LEN || pending > MAX_URI_LEN - _uri.size()) {
            _statusCode = 414;
            log(Level::WARNING, "isTooLong: URI");
            return true;
        }
        break;
    case HTTP_VERSION:
        if (_httpVersion.size() >= MAX_HTTP_LEN || pending > MAX_HTTP_LEN - _httpVersion.size()) {
            _statusCode = 400;
            log(Level::WARNING, "isTooLong: HTTP_VERSION");
            return true;
        }
        break;
    case FIELD_NAME:
        if (_fieldName.size() >= MAX_FIELD_LEN || pending > MAX_FIELD_LEN - _fieldName.size()) {
            _statusCode = 431;
            log(Level::WARNING, "isTooLong: FIELD_NAME");
            return true;
        }
        break;
    case FIELD_VALUE:
        if (_fieldValue.size() >= MAX_FIELD_LEN || pending > MAX_FIELD_LEN - _fieldValue.size()) {
            _statusCode = 431;
            log(Level::WARNING, "isTooLong: FIELD_VALUE");
            return true;
        }
        break;
    case CR:;
    default:;
    }
    return false;
}

bool HttpRequest::extractContent(std::string &recvBuffer, size_t pos) {
    if (isTooLong(pos - _bytesRead) == true)
        return false;
    switch (_currentState) {
    case METHOD:
        _method += recvBuffer.substr(_bytesRead, pos - _bytesRead);
        break;
    case URI:
        _uri += recvBuffer.substr(_bytesRead, pos - _bytesRead);
        break;
    case HTTP_VERSION:
        _httpVersion += recvBuffer.substr(_bytesRead, pos - _bytesRead);
        break;
    case FIELD_NAME:
        _fieldName += recvBuffer.substr(_bytesRead, pos - _bytesRead);
        break;
    case FIELD_VALUE:
        _fieldValue += recvBuffer.substr(_bytesRead, pos - _bytesRead);
        break;
    case CR:;
    default:;
    }
    _bytesRead = pos + 1;
    return true;
}

bool HttpRequest::brokenSyntax(size_t pos, size_t max_pos) {
    if (pos == std::string::npos && max_pos != std::string::npos) {
        _statusCode = 400;
        return 1;
    }
    return 0;
}

void HttpRequest::findSeperator(std::string &recvBuffer,
                                char         seperator,
                                size_t      &pos,
                                size_t      &max_pos) {
    max_pos = recvBuffer.find("\r", _bytesRead);
    pos = recvBuffer.find(seperator, _bytesRead);
}

bool HttpRequest::validMethod() {
    if (!(_method == "GET" || _method == "POST" || _method == "DELETE")) {
        log(Level::WARNING, "invalid method");
        _statusCode = 501;
        return false;
    }
    return true;
}

bool HttpRequest::validUri() {
    if (_uri.size() > 4096) {
        log(Level::WARNING, "invalid URI");
        _statusCode = 414;
        return false;
    }
    if (_uri.find("#") != std::string::npos) {
        log(Level::WARNING, "invalid URI");
        _statusCode = 400;
        return false;
    }
    if (_uri.find("//") != std::string::npos) {
        log(Level::WARNING, "invalid URI");
        _statusCode = 400;
        return false;
    }
    if (_uri.find('\0') != std::string::npos) {
        log(Level::WARNING, "invalid URI");
        _statusCode = 400;
        return false;
    }
    return (true);
}

bool HttpRequest::validateURIPath(std::string &path) {
    if (*(path.begin()) != '/') {
        log(Level::WARNING, "path doesn't begin with '/'");
        return false;
    }
    for (std::string::iterator it = path.begin(); it != path.end(); ++it) {
        if (*it < 33 || *it > 126) {
            log(Level::WARNING, "invalid URI");
            return false;
        }
    }
    if (path.size() > 2 &&
        (path.find("/../") != std::string::npos || path.rfind("/..") == path.size() - 3)) {
        log(Level::WARNING, "escape root sequence found in URI path");
        return false;
    }
    return true;
}

bool HttpRequest::validHttpsVersion() {
    if (_httpVersion.size() != 8 || _httpVersion.compare(0, 5, "HTTP/") != 0 ||
        !isdigit(static_cast<unsigned char>(_httpVersion[5])) || _httpVersion[6] != '.' ||
        !isdigit(static_cast<unsigned char>(_httpVersion[7]))) {
        _statusCode = 400;
        log(Level::WARNING, "HTTP version wrong format");
        return false;
    }
    if (_httpVersion != "HTTP/1.1" && _httpVersion != "HTTP/1.0") {
        _statusCode = 505;
        log(Level::WARNING, "HTTP version not supported");
        return false;
    }
    return true;
}

bool HttpRequest::hasHostHeader() {
    if (_httpVersion == "HTTP/1.0")
        return true;
    std::map<std::string, std::string>::iterator it = _headers.find("host");
    if (it == _headers.end())
        return false;
    return true;
}

bool HttpRequest::hasContentLength() {
    std::map<std::string, std::string>::iterator it = _headers.find("content-length");
    if (it == _headers.end())
        return true;
    if (_currentState == BODY_CHUNKED) {
        log(Level::WARNING, "Header has both transfer-encoding and content-length");
        _statusCode = 400;
        return false;
    }
    const std::string &raw = it->second;
    if (raw.empty() || raw.find_first_not_of("0123456789") != std::string::npos) {
        log(Level::WARNING, "Invalid Content-Length");
        _statusCode = 400;
        return false;
    }
    errno = 0;
    char         *end;
    unsigned long val = std::strtoul(raw.c_str(), &end, 10);
    if (errno == ERANGE || *end != '\0') {
        log(Level::WARNING, "Content-Length out of range");
        _statusCode = 400;
        return false;
    }
    _contentLength = static_cast<size_t>(val);
    if (_contentLength > _clientMaxBody) {
        _statusCode = 413;
        log(Level::WARNING, "Request Entity Too Large");
        return false;
    }
    log(Level::DEBUG, "Starting html body parsing .");
    return true;
}

bool HttpRequest::isChunked() {
    std::map<std::string, std::string>::iterator it = _headers.find("transfer-encoding");
    if (it == _headers.end())
        return true;
    if (_httpVersion == "HTTP/1.0") {
        log(Level::WARNING, "transfer encoding not supported by http 1.0 .");
        return false;
    }
    if (it->second == "chunked") {
        log(Level::DEBUG, "Starting chunked html body parsing .");
        _currentState = BODY_CHUNKED;
        return true;
    }
    log(Level::WARNING, "Unknown transfer-encoding type .");
    return false;
}

bool HttpRequest::validateMandatoryHeaders() {
    if (_method == "POST") {
        if (isChunked() == false)
            return false;
        return (hasHostHeader() && hasContentLength());
    }
    _currentState = METHOD;
    _parsingDone = true;
    return (hasHostHeader());
}

void HttpRequest::init(unsigned int clientMaxBody) { _clientMaxBody = clientMaxBody; }

void HttpRequest::reset() {
    _currentState = METHOD;
    _method.clear();
    _uri.clear();
    _httpVersion.clear();
    _fieldName.clear();
    _fieldValue.clear();
    _contentLength = 0;
    _statusCode = 0;
    _parsingDone = false;
    _headers.clear();
    _body.clear();
    _buffer.clear();
    _bytesNeeded = 0;
    _chunkedBodyState = BYTES;
    _uriData.path.clear();
    _uriData.pathInfo.clear();
    _uriData.query.clear();
    _uriData.extension.clear();
    _headerBytes = 0;
}
