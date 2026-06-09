#ifndef HTTPRESPONSEHPP
#define HTTPRESPONSEHPP

#include "../../ConfigParser/Structs.hpp"
#include "../HttpRequest/HttpRequest.hpp"
#include <map>
#include <string>

struct UriResult {
  int         httpCode;
  std::string path;
  bool        autoindex;
};

enum responseClass {
  INFO = 1,
  SUCCESS = 2,
  REDIR = 3,
  CLIENT_ERR = 4,
  SERVER_ERR = 5
};

class HttpResponse {
public:
  HttpResponse(const t_config& config, const int sid);
  virtual int       build(HttpRequest request);
  void              getReasonPhrase();
  const char*       getResponse();
  std::vector<char> getResponseBody();
  void              reset();
  int               getTimeStamp();

protected:
  void         buildStatusLine();
  int          extractContentType(std::string path);
  void         extractContentLength();
  void         serveErrorPage();
  virtual void addRules();
  void         addMandatoryHeaders();
  void         serveSuccessPage(HttpRequest request);
  virtual void addBody(HttpRequest request, const UriResult& result);
  void         addCookies();
  std::string  getRandomID();

  void getReasonPhraseInfo();
  void getReasonPhraseSuccess();
  void getReasonPhraseRedir();
  void getReasonPhraseClientErr();
  void getReasonPhraseServerErr();

  static bool         isDirectory(std::string& path);
  static unsigned int getLocation(const std::string& match,
                                  const t_server&    serverConfig);
  static void         attachPrefix(const std::string& uri, std::string& path,
                                   t_location& location);
  UriResult processURI(const std::string& uri, const t_server& serverConfig);

  std::map<std::string, std::string>
      _mimeTypes; // should be extracted out of conf file

  // mandatory headers
  size_t      _contentLength; // except status code = 204
  std::string _contentType;
  std::string _timeStamp;

  size_t                             _responseClass;
  std::string                        _reasonPhrase;
  static const std::string           _httpVersion;
  std::map<std::string, std::string> _header;

  const t_config& _config;
  const int       _sid;

  std::string       _response;
  std::vector<char> _responseBody;

  size_t      _statusCode;
  std::string _method;
  std::string _statusCodeStr;
};

#endif
