#ifndef CGI_RESPONSE_CLASS_HPP
#define CGI_RESPONSE_CLASS_HPP

#include "../Client/HttpResponse/HttpResponse.hpp"

#include <sstream>

class CGIResponse : public HttpResponse {
private:
  std::stringstream& _CGIResponseStream;
  size_t             _CGIResponseLen;
  void               addRules();
  void               addBody(HttpRequest request);

public:
  // CGIResponse();
  // ~CGIResponse();
  CGIResponse(std::stringstream& CGIResponseStream, size_t CGIResponseLen);
};

#endif /* CGI_RESPONSE_CLASS_HPP */
