#pragma once
#include "../Client/HttpResponse/HttpResponse.hpp"

#include <sstream>

class CGIResponse : public HttpResponse {
private:
  // std::stringstream& _CGIResponseStream;
  std::string _CGIResponseStr;
  size_t      _CGIResponseLen;
  void        addRules();
  void        addCGIBody(HttpRequest request); //FIX: had to avoid errors to compile. addBody changed in httpresponse

public:
    void              reset();
  int  build(HttpRequest request);
  void   extractStatus(void);
  const std::string& getCGIResponseStr();
  void setCGIResponseStr(const std::string& CGIResponseStr);
  void setCGIResponseLen(size_t len);
  // CGIResponse();
  // ~CGIResponse();
  CGIResponse(std::stringstream& CGIResponseStream, size_t CGIResponseLen,
              const t_config& config, const int sid);
};

