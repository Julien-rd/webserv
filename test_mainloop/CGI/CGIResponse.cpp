#include "CGIResponse.hpp"

#include <cstring>

CGIResponse::CGIResponse(std::stringstream& CGIResponseStream,
                         size_t             CGIResponseLen)
    : _CGIResponseStream(CGIResponseStream), _CGIResponseLen(CGIResponseLen) {}

void CGIResponse::addBody(HttpRequest request) {
  (void)request;
  // std::string str = _CGIResponseStream.str();
  // size_t      bodyPos = str.find("\r\n\r\n");
  // str.erase(0, bodyPos + 4);
  _responseBody.resize(_CGIResponseLen);
  _CGIResponseStream.read(&_responseBody[0], _CGIResponseLen);

  std::ostringstream ss;
  ss << _CGIResponseLen;
  _response += "Content-Length: " + ss.str() + "\r\n";
  _response += "\r\n";
}

void CGIResponse::addRules() {
  if (_statusCode >= 400) {
    _response += "Connection: close\r\n";
    return;
  }
  _response += _response +=
      "Connection: close\r\n"; // or close, maybe also add timeout
  _response += "Cache-Control: max-age=3600\r\n";
  _response += "Custom-CGI-header: the custom value\r\n";
  _response +=
      "Referrer-Policy: strict-origin-when-cross-origin\r\n"; // we could also
                                                              // use a diff one
                                                              // because we dont
                                                              // have https, but
                                                              // it just sends
                                                              // the host url
                                                              // when changing
                                                              // to a different
                                                              // site
  _response +=
      "X-Content-Type-Options: nosniff\r\n"; // makes sure that only the correct
                                             // mime type gets treated, so if
                                             // there is a java script embedded
                                             // in the a png it will not be exec
  _response +=
      "X-Frame-Options: DENY\r\n"; // use our site in a frame on another site,
                                   // prevents clickjacking, theoretically only
                                   // relevant if there are sensitive
                                   // informations or clicks involved, so for
                                   // example we should include this for login
                                   // site etc.
  // _response += "Content Security Policy (CSP)\r\n"; useful against XSS (cross
  // site scripting) -> i dont think it is relevant if we only use static sites,
  // but we can still look into if it is neccessary
  _response += "Content-Security-Policy: default-src 'self'; form-action "
               "'self'\r\n"; // covers script injection and form injection + add
                             // escaping in HTML BODY!!!!! as extra security
                             // layer
}
