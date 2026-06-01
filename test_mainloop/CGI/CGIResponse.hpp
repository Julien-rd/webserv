#ifndef CGI_RESPONSE_CLASS_HPP
#define CGI_RESPONSE_CLASS_HPP

#include "../Client/HttpResponse/HttpResponse.hpp"

#include <sstream>

class CGIResponse : public HttpResponse {
private:
  // std::stringstream& _CGIResponseStream;
  std::string _CGIResponseStr;
  size_t      _CGIResponseLen;
  void        addRules();
  void        addBody(HttpRequest request);

public:
  int  build(HttpRequest request);
  void setCGIResponseStr(const std::string& CGIResponseStr);
  void setCGIResponseLen(size_t len);
  // CGIResponse();
  // ~CGIResponse();
  CGIResponse(std::stringstream& CGIResponseStream, size_t CGIResponseLen);
};

#endif /* CGI_RESPONSE_CLASS_HPP */
