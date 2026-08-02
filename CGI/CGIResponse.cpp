#include "CGIResponse.hpp"

#include <cstring>
#include <iostream>
#include <sstream>

void CGIResponse::init(size_t CGIResponseLen, const t_config *config, const int sid) {
    _CGIResponseLen = CGIResponseLen;
    HttpResponse::init(config, sid);
}

void CGIResponse::reset() {
    _CGIResponseLen = 0;
    _CGIResponseStr.erase();
}

const std::string &CGIResponse::getCGIResponseStr() { return _CGIResponseStr; }

CGIResponse::CGIResponse() {}

size_t CGIResponse::findSeparator(size_t &sepLen) const {
    size_t pos = _CGIResponseStr.find("\r\n\r\n");
    if (pos != std::string::npos) {
        sepLen = 4;
        return pos;
    }
    pos = _CGIResponseStr.find("\n\n");
    if (pos != std::string::npos) {
        sepLen = 2;
        return pos;
    }
    sepLen = 0;
    return std::string::npos;
}

void CGIResponse::extractStatus(void) {
    std::istringstream stream(_CGIResponseStr);
    std::string        line;

    while (std::getline(stream, line)) {
        if (!line.empty() && line[line.size() - 1] == '\r')
            line.erase(line.size() - 1);
        if (line.empty())
            break;
        if (line.compare(0, 7, "Status:") == 0) {
            std::string            value = line.substr(7);
            std::string::size_type start = value.find_first_not_of(" \t");
            if (start != std::string::npos)
                value = value.substr(start);
            std::istringstream(value) >> _statusCode;
            return;
        }
    }
    _statusCode = 200;
}

void CGIResponse::addCGIBody(HttpRequest request) {
    (void) request;

    size_t sepLen = 0;
    size_t separatorPos = findSeparator(sepLen);
    if (separatorPos == std::string::npos) {
        _statusCode = 502;
        return;
    }

    size_t bodyStart = separatorPos + sepLen;
    size_t bodyLen = _CGIResponseStr.size() - bodyStart;

    _responseBody = _CGIResponseStr.substr(bodyStart, bodyLen);

    std::string        headerBlock = _CGIResponseStr.substr(0, separatorPos);
    std::istringstream hs(headerBlock);
    std::string        line;
    while (std::getline(hs, line)) {
        if (!line.empty() && line[line.size() - 1] == '\r')
            line.erase(line.size() - 1);
        if (line.empty())
            continue;
        if (line.compare(0, 7, "Status:") == 0)
            continue;
        if (line.compare(0, 15, "Content-Length:") == 0)
            continue;
        _response += line + "\r\n";
    }

    std::ostringstream ss;
    ss << bodyLen;
    _response += "Content-Length: " + ss.str() + "\r\n";

    _response += "\r\n";
    _response.append(_responseBody.data(), _responseBody.size());
}

void CGIResponse::addRules(const HttpRequest &request) {
    addConnectionHeader(request);
    addCacheHeaders();
    _response += "Custom-CGI-header: the custom value\r\n";
    _response += "Referrer-Policy: strict-origin-when-cross-origin\r\n";
    _response += "X-Content-Type-Options: nosniff\r\n";
    _response += "X-Frame-Options: DENY\r\n";
    _response += "Content-Security-Policy: default-src 'self'; form-action "
                 "'self';img-src 'self' data:;\r\n";
}

void CGIResponse::build(HttpRequest &request) { //fix: detonate this function request ist completely empty, build from scratch
    _statusCode = request.getStatusCode();
    if (_statusCode >= 400) {
        serveErrorPage(request);
        return;
    }
    if (_CGIResponseLen == 0)
        return;
    extractStatus();
    buildStatusLine();
    if (getTimeStamp() == 1)
        return;
    addRules(request);
    if (_statusCode < 400)
        addCGIBody(request);
    if (_statusCode >= 400)
        serveErrorPage(request);
}

void CGIResponse::setCGIResponseStr(const std::string &CGIResponseStr) {
    _CGIResponseStr = CGIResponseStr;
}

void CGIResponse::setCGIResponseLen(size_t len) { _CGIResponseLen = len; }
