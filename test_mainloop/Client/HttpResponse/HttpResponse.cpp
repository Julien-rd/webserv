#include "HttpResponse.hpp"
#include <ctime>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <sys/sendfile.h>

const std::string HttpResponse::_httpVersion = "HTTP/1.1";

HttpResponse::HttpResponse() {
  // _responseBody.resize(1); // safeguard?
  _mimeTypes["html"] = "text/html";
  _mimeTypes["htm"] = "text/html";
  _mimeTypes["css"] = "text/css";
  _mimeTypes["js"] = "application/javascript";
  _mimeTypes["png"] = "image/png";
  _mimeTypes["jpg"] = "image/jpeg";
  _mimeTypes["jpeg"] = "image/jpeg";
  _mimeTypes["ico"] = "image/x-icon";
  _mimeTypes["txt"] = "text/plain";

  std::string webServDir = "..";
  _uri["/ronaldo"] = webServDir + "/mySites/ronaldo.png";
  _uri["/"] = webServDir + "/mySites/index.html";
  _uri["/form"] = webServDir + "/mySites/form.html";
  _uri["/upload"] = webServDir + "/mySites/upload.html";
  _uri["/styles.css"] = webServDir + "/mySites/styles.css";
  _uri["/favicon.ico"] = webServDir + "/mySites/ronaldo.png";
}

std::vector<char> HttpResponse::getResponseBody() { return _responseBody; }

void HttpResponse::reset() {
  _contentLength = 0;
  _contentType.clear();
  _timeStamp.clear();
  _reasonPhrase.clear();
  _header.clear();
  _response.clear();
  _responseBody.clear();
  _statusCodeStr.clear();
}

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
#include <algorithm>
#include <csignal>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>

int HttpResponse::extractContentType(std::string path) {
  size_t pos = path.find_last_of('.');
  if (pos == std::string::npos || pos == 0 || path[pos - 1] == '/')
    return 1;
  // lookup mimetype in map or something like that
  std::string contentType = path.substr(pos + 1);
  std::map<std::string, std::string>::iterator it =
      _mimeTypes.find(contentType);
  if (it == _mimeTypes.end())
    return 1;
  _response += "Content-Type: ";
  _response += it->second;
  _response += "\r\n";
  return 0;
}

void HttpResponse::extractContentLength() {
  std::ostringstream ss;
  ss << _responseBody.size();
  _response += "Content-Length: " + ss.str() + "\r\n";
}

void HttpResponse::addBody(HttpRequest request) {
  std::string path;
  std::string uri = request.getURI();
  std::cout << uri;
  if (uri == "/password.html") {
    serveSuccessPage(request);
    return;
  }
  std::map<std::string, std::string>::iterator it = _uri.find(uri);
  if (it == _uri.end()) {
    _statusCode = 404;
    return; // URI not found
  }
  // check if method is allowed for this uri, if not _statusCode = 405
  path = it->second;
  // end
  std::cout << " == trying to open (" << path.c_str() << ")\n";
  std::fstream htmlPage(path.c_str(), std::ios::in | std::ios::binary);
  if (!htmlPage.is_open()) { // or empty file
    std::cout << "error opening html file: " << strerror(errno);
    return; // error handling
  }
  htmlPage.seekg(0, std::ios::end);
  std::streampos size = htmlPage.tellg();
  htmlPage.seekg(0, std::ios::beg);
  if (size == 0) {
    _statusCode = 404; // check statusCodes
    std::cout << "empty file\n";
    return;
  }
  _responseBody.resize(size);
  htmlPage.read(&_responseBody[0], size);
  if (extractContentType(path) == 1) {
    // mimetype not found
    return;
  }
  extractContentLength();
  _response += "\r\n";
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
  // if get request -> Content length, or chunked header thingy
}

void HttpResponse::buildStatusLine() {
  std::stringstream st;
  st << _statusCode;
  st >> _statusCodeStr;
  _responseClass = _statusCode / 100;
  getReasonPhrase();
  _response = _httpVersion;
  _response += " ";
  _response += _statusCodeStr;
  _response += " ";
  _response += _reasonPhrase;
  _response += "\r\n";
}

void HttpResponse::serveErrorPage() {
  std::ostringstream ss;
  std::stringstream st;
  st << _statusCode;
  st >> _statusCodeStr;
  _responseClass = _statusCode / 100;
  getReasonPhrase();
  std::string htmlBody = "<!DOCTYPE html>\r\n"
                         "<html>\r\n"
                         "    <body>\r\n<h1>" +
                         _statusCodeStr + " " + _reasonPhrase +
                         "</h1>\r\n"
                         "    </body>\r\n"
                         "</html>\r\n";
  _response += "Content-Type: text/html\r\n";
  ss << htmlBody.length();
  _response += "Content-Length: " + ss.str() + "\r\n";
  _response += "\r\n";
  _response += htmlBody; // fix it to char vec
}

void HttpResponse::serveSuccessPage(HttpRequest request) {
  std::ostringstream ss;
  getReasonPhrase();

  std::vector<char> requestBody = request.getBody();

  std::vector<char>::iterator start =
      std::find(requestBody.begin(), requestBody.end(), '=');
  std::vector<char>::iterator end =
      std::find(requestBody.begin(), requestBody.end(), '&');
  std::string username(start + 1, end);

  start = std::find(end, requestBody.end(), '=');
  end = std::find(requestBody.begin(), requestBody.end(), '\r');
  std::string password(start + 1, end);

  std::string htmlBody = "<!DOCTYPE html>\r\n"
                         "<html>\r\n"
                         "    <body>\r\n<h1>"
                         "registration of " +
                         username + " with password: " + password +
                         " successful" // make sure to protect against XSS
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
  if (_statusCode >= 400) {
    serveErrorPage();
    return 1;
  }
  return 0;
}

const char *HttpResponse::getResponse() { return _response.c_str(); }
