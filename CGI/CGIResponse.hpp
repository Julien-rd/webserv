#pragma once
#include "../Client/HttpResponse/HttpResponse.hpp"

#include <sstream>

class CGIResponse : public HttpResponse {
  public:
    CGIResponse();
    CGIResponse(std::stringstream &CGIResponseStream,
                size_t             CGIResponseLen,
                const t_config    &config,
                const int          sid);
    // ~CGIResponse();

    void init(size_t CGIResponseLen, const t_config *config, const int sid);
    void reset();
    void  build(HttpRequest request);
    void extractStatus(void);

    // getters
    const std::string &getCGIResponseStr();

    // setters
    void setCGIResponseStr(const std::string &CGIResponseStr);
    void setCGIResponseLen(size_t len);

  private:
    std::string _CGIResponseStr;
    size_t      _CGIResponseLen;
    void        addRules();
    void addCGIBody(HttpRequest request);  // FIX: had to avoid errors to compile. addBody changed
                                           // in httpresponse
};
