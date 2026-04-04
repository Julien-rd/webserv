#include "HttpResponse.hpp"
#include <ctime>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>

const std::string HttpResponse::_httpVersion = "HTTP/1.1";

HttpResponse::HttpResponse() {
  _mimeTypes["html"] = "text/html";
  _mimeTypes["htm"] = "text/html";
  _mimeTypes["css"] = "text/css";
  _mimeTypes["js"] = "application/javascript";
  _mimeTypes["png"] = "image/png";
  _mimeTypes["jpg"] = "image/jpeg";
  _mimeTypes["jpeg"] = "image/jpeg";
  _mimeTypes["ico"] = "image/x-icon";
  _mimeTypes["txt"] = "text/plain";
}

void HttpResponse::reset() {}

int HttpResponse::getTimeStamp() {
  char buf[1024];
  time_t now = time(0);
  if (now == (time_t)-1)
    return 1;
  struct tm *timeinfo = gmtime(&now);
  if (timeinfo == NULL)
    return 1;
  strftime(buf, sizeof buf, "%a, %d %b %Y %H:%M:%S %Z", timeinfo);
  _timeStamp = buf;
  return 0;
}
#include <csignal>
#include <cstdlib>
#include <iostream>

std::string getHTML() {
  std::string html_body =
      "<!DOCTYPE html>\r\n"
      "<html>\r\n"
      "    <head>\r\n"
      "        <meta charset=\"UTF-8\"/>\r\n"
      "        <title>My site</title>\r\n"
      "        <link rel=\"stylesheet\" href=\"/styles.css\">\r\n"
      "    </head>\r\n"
      "    <body>\r\n"
      "        <h1> yooo whats up! </h1>\r\n"
      "        <h1> yooo whats up pt2! </h1>\r\n"
      "        <h2> yooo whats up diff col mol! </h2>\r\n"
      "    </body>\r\n"
      "</html>\r\n";
  return html_body;
}

std::string getCSS() {
  std::string html_body = "h1 {\r\n"
                          "background-color: green;\r\n"
                          "color: black;\r\n"
                          "}\r\n"

                          "h2 {\r\n"
                          "background-color: pink;\r\n"
                          "color: blue;\r\n"
                          "};\r\n";
  return html_body;
}

// void HttpResponse::addBody(HttpRequest request) {
//   // hard code
//   std::ostringstream ss;
//   std::string htmlBody;
//   std::string mimeType;
//   std::string uri = request.getURI();
//   size_t posDot = uri.rfind('.');
//   size_t posSlash = uri.rfind('/');
//   if (posDot != std::string::npos && posSlash != std::string::npos &&
//       posSlash + 1 < posDot) {
//     mimeType = uri.substr(posSlash + 1);
//     if (_mimeTypes.find(mimeType) == _mimeTypes.end()) {
//       std::cout << "mimeType not found\n";
//       return;
//     }
//     if (mimeType == "html")
//       htmlBody = getHTML();
//     else if (mimeType == "css")
//       htmlBody = getHTML();
//     _response += "Content-Type" + _mimeTypes[mimeType] + "\r\n";
//     ss << htmlBody.length();
//     _response += "Content-Length: " + ss.str() + "\r\n";
//     _response += "\r\n";
//     _response += htmlBody;
//   } else
//     std::cout << "FASILLELELEL\n";
// } 
//->>> needs URI Path logic but is the newest version

void HttpResponse::addBody(HttpRequest request) {
  // hard code
  std::ostringstream ss;
  std::string html_body;
  if (request.getURI() == "/") {
    html_body = getHTML();
    _response += "Content-Type: text/html\r\n"; // TODO make it more dynamic with mime types
  } else {
    html_body = getCSS();
    // "text/css; charset=UTF-8");
    _response += "Content-Type: text/css\r\n";
  }
  ss << html_body.length();
  _response += "Content-Length: " + ss.str() + "\r\n";
  _response += "\r\n";
  _response += html_body;
}

void HttpResponse::addRules() {
  if (_statusCode >= 400) {
    _response += "Connection: close\r\n";
    return;
  }
  _response += "Connection: keep-alive\r\n"; // or close, maybe also add timeout
  _response += "Cache-Control: max-age=3600\r\n";
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

void HttpResponse::addMandatoryHeaders() {
  _response += "Date: " + _timeStamp + "\r\n"; // apparently not mandatory
}

void HttpResponse::buildStatusLine() {
  std::stringstream st;
  st << _statusCode;
  std::string statusCodeStr;
  st >> statusCodeStr;
  _responseClass = _statusCode / 100;
  getReasonPhrase();
  _response = _httpVersion;
  _response += " ";
  _response += statusCodeStr;
  _response += " ";
  _response += _reasonPhrase;
  _response += "\r\n";
}

void HttpResponse::serveErrorPage() {
  std::stringstream st;
  st << _statusCode;
  std::string statusCodeStr;
  st >> statusCodeStr;
  std::ostringstream ss;
  std::string htmlBody = "<!DOCTYPE html>\r\n"
                         "<html>\r\n"
                         "    <body>\r\n<h1>" +
                         statusCodeStr + " " + _reasonPhrase +
                         "</h1>\r\n"
                         "    </body>\r\n"
                         "</html>\r\n";
  _response += "Content-Type: text/html\r\n";
  ss << htmlBody.length();
  _response += "Content-Length: " + ss.str() + "\r\n";
  _response += "\r\n";
  _response += htmlBody;
}

int HttpResponse::build(HttpRequest request) {
  _statusCode = request.getStatusCode();
  buildStatusLine(); // only mandatory part
  if (getTimeStamp() == 1)
    return 1;
  addMandatoryHeaders();
  addRules();
  if (_statusCode < 400)
    addBody(request);
  else
    serveErrorPage();
  return 0;
}

const char *HttpResponse::getResponse() { return _response.c_str(); }
