#ifndef HTTPRESPONSEHPP
#define HTTPRESPONSEHPP

#include "../HttpRequest/HttpRequest.hpp"
#include <map>
#include <string>

enum responseClass {
  INFO = 1,
  SUCCESS = 2,
  REDIR = 3,
  CLIENT_ERR = 4,
  SERVER_ERR = 5
};


class HttpResponse {
public:
  HttpResponse();
  int build(HttpRequest request);
  void getReasonPhrase();
  const char *getResponse();
  std::vector<char> getResponseBody();
  void reset();
  int getTimeStamp();

private:
  void buildStatusLine();
  int extractContentType(std::string path);
  void extractContentLength();
  void serveErrorPage();
  void addRules();
  void addMandatoryHeaders();
  void serveSuccessPage(HttpRequest request);
  void addBody(HttpRequest request);

  void getReasonPhraseInfo();
  void getReasonPhraseSuccess();
  void getReasonPhraseRedir();
  void getReasonPhraseClientErr();
  void getReasonPhraseServerErr();

  std::map<std::string, std::string> _mimeTypes; // should be extracted out of conf file
  std::map<std::string, std::string> _uri;

  //mandatory headers
  size_t _contentLength; // except status code = 204
  std::string _contentType;
  std::string _timeStamp;

  size_t _responseClass;
  std::string _reasonPhrase;
  static const std::string _httpVersion;
  std::map<std::string, std::string> _header;

  std::string _response;
  std::vector<char> _responseBody;

  size_t _statusCode;
  std::string _statusCodeStr;
};

#endif

