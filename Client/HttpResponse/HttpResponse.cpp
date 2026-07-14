#include "HttpResponse.hpp"
#include "../../Utils/Macros.hpp"

#include <algorithm>
#include <cstddef>
#include <dirent.h>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <sys/types.h>

#include <csignal>
#include <cstdlib>
#include <cstring>
#include <ctime>

#include <sys/sendfile.h>

#include "HttpResponse.hpp"
#include <cstddef>
#include <string>
#include <sys/stat.h>
#include <unistd.h>

bool HttpResponse::isDirectory(std::string& path) {
  struct stat stats;
  stat(path.c_str(), &stats);
  if (S_ISDIR(stats.st_mode))
    return true;
  return false;
}

unsigned int HttpResponse::getLocation(const std::string& match,
                                       const t_server&    serverConfig) {
  unsigned int longestMatch = 0;
  unsigned int ret = 0;

  for (unsigned int i = 0; i < serverConfig.locations.size(); ++i) {
    std::string identifier = serverConfig.locations.at(i).name;
    if (match.compare(0, identifier.size(), identifier) == 0 &&
        identifier.size() > longestMatch) {
      longestMatch = identifier.size();
      ret = i;
    }
  }
  return ret;
}

void HttpResponse::attachPrefix(const std::string& uri, std::string& path,
                                t_location& location) {
  if (!location.alias.empty())
    path = ROOT_FOLDER + location.alias + uri.substr(location.name.length());
  else if (!location.root.empty())
    path = ROOT_FOLDER + location.root + uri;
  else
    path = ROOT_FOLDER + uri;
}

bool HttpResponse::methodAllowed(unsigned int index, const std::vector<t_location>& locations) {
    bool rootEmpty = !(*locations.begin()).allowMethods.size();
    bool currentEmpty = !locations.at(index).allowMethods.size();
    
    if (!currentEmpty) {
        const t_location& current = locations.at(index);
        return std::find(current.allowMethods.begin(), current.allowMethods.end(),
            _method) != current.allowMethods.end() ? 1 : 0;
    }
    if (!rootEmpty) {
        const t_location& root = *locations.begin();
        return std::find(root.allowMethods.begin(), root.allowMethods.end(),
            _method) != root.allowMethods.end() ? 1 : 0;
    }
    return true;
}

/**
 * @brief Uses the URI using config data to return the resolved path and http
 * code.
 * @param uri          The request URI from the client.
 * @param serverConfig The server configuration associated with this client.
 * @return UriResult - httpCode to determine action, path to execute action
 * with, bool for autoindex.
 */
UriResult HttpResponse::processURI(const std::string& uri) {
        UriResult   result;
        struct stat stats;
        result.httpCode = 200;
        result.autoindex = false;
        const t_server& serverConfig = _config.servers.at(_sid);
        unsigned int index = getLocation(uri, serverConfig);
        t_location   location = serverConfig.locations.at(index);
        attachPrefix(uri, result.path, location);
        if (stat(result.path.c_str(), &stats) == -1) {
            if (!location.redirect.second.empty()) {
                result.httpCode = location.redirect.first;
                result.path = location.redirect.second;
                return result;
            }
            result.httpCode = 404;
        } else if (S_ISDIR(stats.st_mode)) {
            if (!uri.empty() && uri[uri.size() - 1] != '/') {
                result.httpCode = 301;
                result.path = uri + '/';
            } else if (!location.redirect.second.empty()) {
                result.httpCode = location.redirect.first;
                result.path = location.redirect.second;
                return result;
            } else if (!methodAllowed(index, serverConfig.locations)) {
                result.httpCode = 405;
            } else if (!location.tryFiles.empty()) {
                std::string tmp;
                for (size_t i = 0; i < location.tryFiles.size(); ++i) {
                    tmp = result.path + location.tryFiles.at(i);
                    if (access(tmp.c_str(), F_OK | R_OK) == 0) {
                        result.path = tmp;
                        return result;
                    }
                }
                result.httpCode = 404;
            } else if (!location.index.empty())
                result.path += location.index;
            else if (access((result.path + "index.html").c_str(), F_OK | R_OK) == 0)
                result.path += "index.html";
            else if (location.autoindex)
                result.autoindex = true;
            else 
                result.httpCode = 404;
        }
        std::cout << result.path << "\n";
        std::cout << result.httpCode << "\n";
        return result;
    }

const std::string HttpResponse::_httpVersion = "HTTP/1.1";

HttpResponse::HttpResponse(const t_config& config, const int sid)
    : _config(config), _sid(sid) {
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
  char   buf[1024];
  time_t now = time(0);
  if (now == (time_t)-1)
    return 1;
  struct tm* timeinfo = gmtime(&now);
  if (timeinfo == NULL)
    return 1;
  strftime(buf, sizeof buf, "%a, %d %b %Y %H:%M:%S %Z", timeinfo);
  _timeStamp = buf;
  return 0;
}

int HttpResponse::extractContentType(std::string path) {
  size_t pos = path.find_last_of('.');
  if (pos == std::string::npos || pos == 0 || path[pos - 1] == '/')
    return 1;
  // TODO lookup mimetype in map or something like that
  std::string contentType = path.substr(pos + 1);
  std::map<std::string, std::string>::iterator it =
      _mimeTypes.find(contentType);
  if (it == _mimeTypes.end()) { // TODO: fix, infinitely loads on fail
    return 1;
  }
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

std::string autoindex(const std::string& path, const std::string& uri) {
  std::string file;
  file = "<!DOCTYPE html>\r\n"
         "<html>\r\n"
         "<body>\r\n\r\n"
         "<h1>Index of " +
         uri + "</h1>\r\n";
  DIR* dir = opendir(path.c_str());
  if (!dir)
    return NULL;
  struct dirent* dr = readdir(dir);
  while (dr) {
    if (std::string(".").compare(dr->d_name))
      file += "<p><a href=\"" + uri + dr->d_name + "\">" + dr->d_name +
              "</a></p>\r\n";
    dr = readdir(dir);
  }
  file += "\r\n\r\n</body>\r\n</html>";
  return file;
}
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

void HttpResponse::addBody(HttpRequest request, const UriResult& result) {

  std::fstream htmlPage;
  std::string  uri = request.getURI();
  std::string  autoindexHtml;
  if (result.autoindex == true) {
      autoindexHtml = autoindex(result.path, uri);
      std::cout << "\n{" + autoindexHtml << "}\n";
      std::stringstream here(autoindexHtml);
      _responseBody.resize(autoindexHtml.size());
      here.read(&_responseBody[0], autoindexHtml.size());
      _response += "Content-Type: text/html\r\n";
  }
  else {
      htmlPage.open(result.path.c_str());
      if (!htmlPage.is_open()) {
        std::cout << "error opening html file: " << strerror(errno);
        _statusCode = 404;
        return;
      }
      htmlPage.seekg(0, std::ios::end);
      std::streampos size = htmlPage.tellg(); // TODO change this. we can't use seek
      htmlPage.seekg(0, std::ios::beg);
      // if (size == 0) {
      //     _statusCode = 404; // check statusCodes
      //     std::cout << "empty file\n";
      //     return;
      // }
      _responseBody.resize(size);
      htmlPage.read(&_responseBody[0], size);
      if (extractContentType(result.path) == 1) {
          return;
      }
  }
  extractContentLength();
  _response += "\r\n";
}

void HttpResponse::addRules() {
  // if (_statusCode >= 300) {
  //   // _response += "Connection: close\r\n";
  //   return;
  // }
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
  // 
  _response += "Content-Security-Policy: default-src 'self'; style-src 'self' 'unsafe-inline'; form-action 'self'\r\n";
                            // covers script injection and form injection + add
                             // escaping in HTML BODY!!!!! as extra security
                             // layer
}

void HttpResponse::addMandatoryHeaders() { //FIX: Will there be more mandatories? otherwise just remove
    if (getTimeStamp() != 1)
        _response += "Date: " + _timeStamp + "\r\n";
  // if get request -> Content length, or chunked header thingy
}

void HttpResponse::addRedirectHeaders(const std::string& path) {
    _response += "Location: " + path + "\r\n";
    _response += "Content-Length: 0\r\n\r\n";
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
  std::stringstream  st;
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

void HttpResponse::serveSuccessPage(
    HttpRequest
        request) { // FIX: why is this function never used? or where is it used
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
    
  UriResult    result;
  std::fstream htmlPage;
  std::string  uri = request.getURI();
  std::string  autoindexHtml;
  
  _statusCode = request.getStatusCode();
  if (_statusCode < 400) {
      _method = request.getMethod(); //FIX: Maybe add request to the response class so we can check for this elsewhere
      result = processURI(uri);
      _statusCode = result.httpCode;
  }
  
  buildStatusLine(); // only mandatory part
  addMandatoryHeaders();
  addRules();
  
  if (_statusCode > 299 && _statusCode < 400)
      addRedirectHeaders(result.path);
  else if (_statusCode >= 400) {
    serveErrorPage();
    return 1; //FIX: What happens at 1? 400 is totally fine
  }
  else
      addBody(request, result);
  return 0;
}

const char* HttpResponse::getResponse() { return _response.c_str(); }
