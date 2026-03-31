#include "HttpResponse.hpp"
#include <ctime>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>

const std::string HttpResponse::_httpVersion = "HTTP/1.1";

HttpResponse::HttpResponse() {}

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

void HttpResponse::addBody() {
  // hard code
  std::ostringstream ss;
  std::string html_body;
  if (request.getURI() == "/") {
    html_body = getHTML();
    _response += "Content-Type: text/html\r\n";
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
  _response += "Connection: keep-alive\r\n"; // or close
  _response += "Cache-Control: max-age=3600\r\n";
}

void HttpResponse::addMandatoryHeaders() {
  _response += "Date: " + _timeStamp + "\r\n";
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

int HttpResponse::build(HttpRequest request) {
  _statusCode = request.getStatusCode();
  buildStatusLine();
  if (getTimeStamp() == 1)
    return 1;
  addMandatoryHeaders();
  addRules();
  addBody();
  return 0;
}

const char *HttpResponse::getResponse() { return _response.c_str(); }