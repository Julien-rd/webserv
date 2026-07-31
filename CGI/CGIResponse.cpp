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

void CGIResponse::extractStatus(void) {
    std::istringstream stream(_CGIResponseStr);
    std::string        line;

    while (std::getline(stream, line)) {
        if (line.empty() || line == "\r")
            break;
        if (line.substr(0, 7) == "Status:") {
            std::string value = line.substr(8);
            std::stringstream(value) >> _statusCode;
            return;
        }
    }
    _statusCode = 200;
    return;
}

CGIResponse::CGIResponse() {}

void CGIResponse::addCGIBody(HttpRequest request) {
    (void) request;
    size_t separatorPos = _CGIResponseStr.find("\r\n\r\n");  // fix what if not found here
    if (separatorPos == std::string::npos) { //fix: test and move this out of here maybe
        _statusCode = 502;
        return;
    }
    _responseBody.resize(_CGIResponseLen - 4 - separatorPos);
    std::stringstream responseStream(_CGIResponseStr);
    responseStream.seekg(separatorPos + 4);
    responseStream.read(&_responseBody[0], _CGIResponseLen - 4 - separatorPos);

    std::ostringstream ss;
    ss << _CGIResponseLen - 4 - separatorPos;
    _response += "Content-Length: " + ss.str() + "\r\n";
    _response += _CGIResponseStr.substr(0, separatorPos + 4);
    _response.append(&_responseBody[0], _CGIResponseLen - 4 - separatorPos);
}

void CGIResponse::addRules() {
    // if (_statusCode >= 400) {
    //   // _response += "Connection: close\r\n";
    //   return;
    // }
    _response += "Connection: keep-alive\r\n";  // or close, maybe also add timeout
    _response += "Cache-Control: max-age=3600\r\n";
    _response += "Custom-CGI-header: the custom value\r\n";
    _response += "Referrer-Policy: strict-origin-when-cross-origin\r\n";  // we could also
                                                                          // use a diff one
                                                                          // because we dont
                                                                          // have https, but
                                                                          // it just sends
                                                                          // the host url
                                                                          // when changing
                                                                          // to a different
                                                                          // site
    _response += "X-Content-Type-Options: nosniff\r\n";  // makes sure that only the correct
                                                         // mime type gets treated, so if
                                                         // there is a java script embedded
                                                         // in the a png it will not be exec
    _response += "X-Frame-Options: DENY\r\n";            // use our site in a frame on another site,
                                               // prevents clickjacking, theoretically only
                                               // relevant if there are sensitive
                                               // informations or clicks involved, so for
                                               // example we should include this for login
                                               // site etc.
    // _response += "Content Security Policy (CSP)\r\n"; useful against XSS (cross
    // site scripting) -> i dont think it is relevant if we only use static sites,
    // but we can still look into if it is neccessary
    _response += "Content-Security-Policy: default-src 'self'; form-action "
                 "'self';img-src 'self' data:;\r\n";  // covers script injection and form injection
                                                      // + add escaping in HTML BODY!!!!! as extra
                                                      // security layer
}

void CGIResponse::build(HttpRequest request) {  // fix: make this similar to og build() again
    _statusCode = request.getStatusCode();
    if (_statusCode >= 400) {
        serveErrorPage();
        return;
    }
    if (_CGIResponseLen == 0)
        return;  // fix: what to do here
    extractStatus();
    buildStatusLine();  // only mandatory part
    if (getTimeStamp() == 1)
        return;  // fix: what to do here
    addMandatoryHeaders();
    addRules();
    if (_statusCode < 400)
        addCGIBody(request);
    if (_statusCode >= 400)
        serveErrorPage();
}

void CGIResponse::setCGIResponseStr(const std::string &CGIResponseStr) {
    _CGIResponseStr = CGIResponseStr;
}

void CGIResponse::setCGIResponseLen(size_t len) { _CGIResponseLen = len; }
