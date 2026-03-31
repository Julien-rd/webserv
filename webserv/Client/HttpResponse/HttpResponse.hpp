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
  void reset();
  int getTimeStamp();

private:
  void buildStatusLine();
  void addRules();
  void addMandatoryHeaders();
  void addBody();

  void getReasonPhraseInfo();
  void getReasonPhraseSuccess();
  void getReasonPhraseRedir();
  void getReasonPhraseClientErr();
  void getReasonPhraseServerErr();


  //mandatory headers
  size_t _contentLength; // except status code = 204
  std::string _contentType;
  std::string _timeStamp;

  size_t _responseClass;
  std::string _reasonPhrase;
  static const std::string _httpVersion;
  std::map<std::string, std::string> _header;
  std::string _response;
  size_t _statusCode;
};

#endif