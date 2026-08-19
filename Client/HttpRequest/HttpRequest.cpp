#include "HttpRequest.hpp"

#include "../../Logger/Logger.hpp"
#include "../../Utils/Macros.hpp"

#include <iostream>
#include <sstream>
#include <string>
#include <vector>

HttpRequest::HttpRequest(size_t clientMaxBody)
        : _currentState(METHOD)
        , _contentLength(0)
        , _buffer("")
        , _chunkedBodyState(BYTES)
        , _bytesNeeded(0)
        , _bytesRead(0)
        , _statusCode(0)
        , _parsingDone(false)
        , _clientMaxBody(clientMaxBody)
        , _headerBytes(0) {}

HttpRequest::HttpRequest()
        : _currentState(METHOD)
        , _contentLength(0)
        , _buffer("")
        , _chunkedBodyState(BYTES)
        , _bytesNeeded(0)
        , _bytesRead(0)
        , _statusCode(0)
        , _parsingDone(false)
        , _clientMaxBody(2)
        , _headerBytes(0) {}

const std::vector<char> &HttpRequest::getBody() const { return _body; }

HttpRequest::HttpRequest(const HttpRequest &obj) {
    _method = obj._method;
    _uri = obj._uri;
    _headers = obj._headers;
    _currentState = obj._currentState;
    _chunkedBodyState = obj._chunkedBodyState;
    _contentLength = obj._contentLength;
    _uriData = obj._uriData;
    _httpVersion = obj._httpVersion;
    _fieldName = obj._fieldName;
    _fieldValue = obj._fieldValue;
    _bytesRead = obj._bytesRead;
    _statusCode = obj._statusCode;
    _parsingDone = obj._parsingDone;
    _body = obj._body;
    _clientMaxBody = obj._clientMaxBody;
    _buffer = obj._buffer;
    _bytesNeeded = obj._bytesNeeded;
    _headerBytes = obj._headerBytes;
}

const HttpRequest &HttpRequest::operator=(const HttpRequest &obj) {
    if (&obj == this) {
        return *this;
    }
    _method = obj._method;
    _uri = obj._uri;
    _headers = obj._headers;
    _currentState = obj._currentState;
    _contentLength = obj._contentLength;
    _chunkedBodyState = obj._chunkedBodyState;
    _uriData = obj._uriData;
    _httpVersion = obj._httpVersion;
    _fieldName = obj._fieldName;
    _fieldValue = obj._fieldValue;
    _bytesRead = obj._bytesRead;
    _statusCode = obj._statusCode;
    _parsingDone = obj._parsingDone;
    _body = obj._body;
    _clientMaxBody = obj._clientMaxBody;
    _buffer = obj._buffer;
    _bytesNeeded = obj._bytesNeeded;
    _headerBytes = obj._headerBytes;
    return *this;
}

void HttpRequest::print() {
    std::cout << "---------------------RequestLine---------------------\n";
    std::cout << "Method: [" << _method << "]\n"
              << "URI: [" << _uri << "]\n"
              << "HTTP_VERSION: [" << _httpVersion << "]\n";
    std::cout << "\n---------------------Headers---------------------\n";
    for (std::map<std::string, std::string>::iterator it = _headers.begin(); it != _headers.end();
         ++it) {
        std::cout << "Fieldname: [" << it->first << "], Fieldvalue: [" << it->second << "]" << "\n";
    }
}
bool HttpRequest::parsingDone() { return _parsingDone; }

bool HttpRequest::validNewLine(std::string &recvBuffer) {
    if (!recvBuffer.empty() && recvBuffer[_bytesRead] == '\n')
        return 0;
    else {
        _statusCode = 400;
        return 1;
    }
}

#include <cstdlib>
int HttpRequest::parseRequestLine(std::string &recvBuffer) {
    size_t pos;
    size_t max_pos;
    switch (_currentState) {
    case METHOD:
        findSeperator(recvBuffer, ' ', pos, max_pos);
        if (brokenSyntax(pos, max_pos))
            return 1;
        if (pos == std::string::npos) {
            if (isTooLong(recvBuffer.size() - _bytesRead) == true)
                return 1;
            _method += recvBuffer.substr(_bytesRead);
            _bytesRead = recvBuffer.size();
            break;
        }
        if (extractContent(recvBuffer, pos) == false)
            return 1;
        if (validMethod() == false)
            return 1;
        _currentState = URI;
        /* fall through */
    case URI:;
        findSeperator(recvBuffer, ' ', pos, max_pos);
        if (brokenSyntax(pos, max_pos))
            return 1;
        if (pos == std::string::npos) {
            if (isTooLong(recvBuffer.size() - _bytesRead) == true)
                return 1;
            _uri += recvBuffer.substr(_bytesRead);
            _bytesRead = recvBuffer.size();
            break;
        }
        if (extractContent(recvBuffer, pos) == false)
            return 1;
        if (validUri() == false)
            return 1;
        _currentState = HTTP_VERSION;
        /* fall through */
    case HTTP_VERSION:
        pos = recvBuffer.find("\r", _bytesRead);
        if (pos == std::string::npos) {
            if (isTooLong(recvBuffer.size() - _bytesRead) == true)
                return 1;
            _httpVersion += recvBuffer.substr(_bytesRead);
            _bytesRead = recvBuffer.size();
            break;
        }
        if (extractContent(recvBuffer, pos) == false)
            return 1;
        if (validHttpsVersion() == false)
            return 1;
        _currentState = CR;
        /* fall through */
    case CR:
        pos = recvBuffer.find("\n", _bytesRead);
        if (_bytesRead >= recvBuffer.size())
            return 0;
        if (validNewLine(recvBuffer) == 1)
            return 1;
        if (extractContent(recvBuffer, pos) == false)
            return 1;
        _currentState = FIELD_NAME;
        log(Level::DEBUG, "Requestline parsing done.");
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

int HttpRequest::parseHeaders(std::string &recvBuffer) {
    size_t pos;
    size_t max_pos;
    while (_bytesRead < recvBuffer.length()) {
        switch (_currentState) {
        case FIELD_NAME:
            findSeperator(recvBuffer, ':', pos, max_pos);
            if (_bytesRead < recvBuffer.size() && recvBuffer[_bytesRead] == '\r') {
                _bytesRead += 1;
                _currentState = EOH;
                break;
            }
            if (pos == std::string::npos) {
                if (isTooLong(recvBuffer.size() - _bytesRead) == true)
                    return 1;
                _fieldName += recvBuffer.substr(_bytesRead);
                _bytesRead = recvBuffer.size();
                return 0;
            }
            if (extractContent(recvBuffer, pos) == false)
                return 1;
            if (containsWhiteSpaces() == true)
                return 1;
            _currentState = FIELD_VALUE;
            /* fall through */
        case FIELD_VALUE:
            pos = recvBuffer.find("\r", _bytesRead);
            if (pos == std::string::npos) {
                if (isTooLong(recvBuffer.size() - _bytesRead) == true)
                    return 1;
                _fieldValue += recvBuffer.substr(_bytesRead);
                _bytesRead = recvBuffer.size();
                return 0;
            }
            if (extractContent(recvBuffer, pos) == false)
                return 1;
            trim();
            if (addHeader() == false)
                return 1;
            _currentState = CR;
            /* fall through */
        case CR:
            pos = recvBuffer.find("\n", _bytesRead);
            if (_bytesRead >= recvBuffer.size())
                return 0;
            if (validNewLine(recvBuffer) == 1)
                return 1;
            if (extractContent(recvBuffer, pos) == false)
                return 1;
            _currentState = FIELD_NAME;
            break;
        case EOH:  // TODO what if \r \n are sent seperatly
            if (_bytesRead >= recvBuffer.size())
                return 0;
            if (validNewLine(recvBuffer) == 1) {
                log(Level::WARNING, "parseHttpRequest: parseHeaders");
                return 1;
            }
            ++_bytesRead;
            _currentState = BODY;
            log(Level::DEBUG, "Header parsing done.");
            if (validateMandatoryHeaders() == false)
                return 1;
            /* fall through */
        default:
            return 0;
        }
    }
    return 0;
}

bool parseHexSize(std::string s, size_t &out) {
    size_t pos = s.find(";");
    if (pos != std::string::npos)
        s.erase(pos);
    if (s.empty())
        return false;
    for (std::string::size_type i = 0; i < s.size(); ++i) {
        if (!isxdigit(static_cast<unsigned char>(s[i])))
            return false;
    }
    std::istringstream iss(s);
    iss >> std::hex >> out;
    if (iss.fail())
        return false;
    char leftover;
    if (iss >> leftover)
        return false;
    return true;
}

void HttpRequest::parseBody(std::string recvBuffer) {
    _bytesNeeded = _contentLength - _body.size();
    std::string::iterator start = recvBuffer.begin() + _bytesRead;
    std::string::iterator end = recvBuffer.end();
    size_t                len = recvBuffer.length() - _bytesRead;
    if (_bytesNeeded > len) {
        _body.insert(_body.end(), start, end);
        _bytesRead += len;
    } else {
        _body.insert(_body.end(), start, start + _bytesNeeded);
        _parsingDone = true;
        _statusCode = 200;
        _bytesRead += _bytesNeeded;
        log(Level::DEBUG, "completed HTML body parsing");
    }
}

int HttpRequest::parseChunkedBody(const std::string &recvBuffer) {
    size_t trailerBytes = 0;

    _buffer += recvBuffer.substr(_bytesRead);
    _bytesRead = recvBuffer.size();

    while (true) {
        if (_chunkedBodyState == BYTES) {
            size_t pos = _buffer.find("\r\n");
            if (pos == std::string::npos) {
                if (_buffer.size() > MAX_CHUNK_LINE) {
                    _parsingDone = true;
                    _statusCode = 400;
                    log(Level::WARNING, "parseChunkedBody: chunk size line too long.");
                    return 1;
                }
                return 0;
            }
            if (pos > MAX_CHUNK_LINE) {
                _parsingDone = true;
                _statusCode = 400;
                log(Level::WARNING, "parseChunkedBody: chunk size line too long.");
                return 1;
            }
            size_t cutOff = _buffer.find(";");
            if (cutOff == std::string::npos || cutOff > pos)
                cutOff = pos;
            if (parseHexSize(_buffer.substr(0, cutOff), _bytesNeeded) == false) {
                _parsingDone = true;
                _statusCode = 400;
                log(Level::WARNING, "parseChunkedBody: invalid chunk size.");
                return 1;
            }
            if (_body.size() >= _clientMaxBody || _bytesNeeded > _clientMaxBody - _body.size()) {
                _parsingDone = true;
                _statusCode = 413;
                log(Level::WARNING, "parseChunkedBody: body exceeds clientMaxBody.");
                return 1;
            }
            _buffer.erase(0, pos + 2);
            _chunkedBodyState = (_bytesNeeded == 0) ? _EOF : LINE;
        } else if (_chunkedBodyState == LINE) {
            if (_buffer.size() < _bytesNeeded + 2)
                return 0;
            if (_buffer[_bytesNeeded] != '\r' || _buffer[_bytesNeeded + 1] != '\n') {
                _parsingDone = true;
                _statusCode = 400;
                log(Level::WARNING, "parseChunkedBody: bytes announced != bytes received.");
                return 1;
            }
            _body.insert(_body.end(), _buffer.begin(), _buffer.begin() + _bytesNeeded);
            _buffer.erase(0, _bytesNeeded + 2);
            _chunkedBodyState = BYTES;
        } else if (_chunkedBodyState == _EOF) {
            size_t pos = _buffer.find("\r\n");
            if (pos == std::string::npos) {
                if (_buffer.size() > MAX_TRAILER_SUM) {
                    _parsingDone = true;
                    _statusCode = 400;
                    log(Level::WARNING, "parseChunkedBody: trailer block too long.");
                    return 1;
                }
                return 0;
            }
            if (pos == 0) {
                _buffer.erase(0, 2);
                _bytesRead = recvBuffer.size() - _buffer.size();
                _parsingDone = true;
                _statusCode = 200;
                log(Level::DEBUG, "parseChunkedBody: body parsing done.");
                return 0;
            }
            if (pos > MAX_TRAILER_SUM - trailerBytes) {
                _parsingDone = true;
                _statusCode = 400;
                log(Level::WARNING, "parseChunkedBody: trailer block too long.");
                return 1;
            }
            trailerBytes += pos;
            _buffer.erase(0, pos + 2);
        }
    }
}

int HttpRequest::bodyMode(std::string recvBuffer) {
    if (_currentState == BODY)
        parseBody(recvBuffer);
    if (_currentState == BODY_CHUNKED) {
        if (parseChunkedBody(recvBuffer) == 1)
            return 1;
    }
    return 0;
}

int HttpRequest::parseHttpRequest(std::string &recvBuffer, size_t bytes_read) {
    _bytesRead = bytes_read;
    _parsingDone = false;
    if (parseRequestLine(recvBuffer) == 1) {
        log(Level::WARNING, "parseHttpRequest: parseRequestLine");
        return 1;
    }
    if (parseHeaders(recvBuffer) == 1) {
        log(Level::WARNING, "parseHttpRequest: parseHeaders");
        return 1;
    }
    if (bodyMode(recvBuffer) == 1) {
        log(Level::WARNING, "parseHttpRequest: body no tea");
        return 1;
    }
    return 0;
}

std::string percentDecode(const std::string &encoded, bool isQuery) {
    std::string result;
    for (size_t i = 0; i < encoded.size(); ++i) {
        if (encoded[i] == '%' && i + 2 < encoded.size()) {
            int                val;
            std::istringstream hex(encoded.substr(i + 1, 2));
            hex >> std::hex >> val;
            result += static_cast<char>(val);
            i += 2;
        } else if (isQuery && encoded[i] == '+') {
            result += ' ';  // only in query strings, not paths
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
        _statusCode = 400;
        return 1;
    }
    _uriData.query = (qmark != std::string::npos) ? _uri.substr(qmark + 1) : "";

    size_t dot = path.rfind('.');
    size_t lastSlash = path.rfind('/');
    if (lastSlash == 0) {
        path.append("/");
        lastSlash = path.size() - 1;
    }
    if (dot != std::string::npos && dot < lastSlash) {
        size_t extEnd = path.find('/', dot);
        _uriData.extension = path.substr(dot, extEnd - dot);
        _uriData.path = path.substr(0, extEnd);
        _uriData.pathInfo = (extEnd != std::string::npos) ? path.substr(extEnd) : "";
    } else {
        if (dot != std::string::npos)
            _uriData.extension = path.substr(dot);
        _uriData.path = path;
    }
    _uriData.query = percentDecode(_uriData.query, true);
    _uri = _uriData.path;
    return 0;
}
