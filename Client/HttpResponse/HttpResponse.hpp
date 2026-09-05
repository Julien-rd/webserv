#pragma once
#include "../../ConfigParser/Structs.hpp"
#include "../HttpRequest/HttpRequest.hpp"

#include <map>
#include <string>

struct UriResult {
    int         httpCode;
    std::string path;
    bool        autoindex;
};

enum responseClass { INFO = 1, SUCCESS = 2, REDIR = 3, CLIENT_ERR = 4, SERVER_ERR = 5 };

class HttpResponse {
  public:
    HttpResponse();
    virtual void      build(HttpRequest &request);
    void              getReasonPhrase();
    const char       *getResponse() const;
    std::vector<char> getResponseBody() const;
    std::vector<char> getFullResponse() const;

    void         init(const t_config *config, const int sid);
    virtual void reset();
    int          getTimeStamp();
    void         setConnection(bool keepAlive);
    bool         keepConnection() const;

  protected:
    // Fix: should be extracted out of conf file
    std::map<std::string, std::string> _mimeTypes;
    // what does this comment mean or what does it lead to? : except status code 204
    std::string                        _contentType;
    std::string                        _timeStamp;
    std::string                        _reasonPhrase;
    static const std::string           _httpVersion;
    const t_config                    *_config;
    int                                _sid;
    size_t                             _statusCode;
    size_t                             _responseClass;
    size_t                             _contentLength;
    std::string                        _response;
    std::vector<char>                  _responseBody;
    std::string                        _method;
    std::string                        _statusCodeStr;
    bool                               _keepAlive;

    bool                methodAllowed(unsigned int index, const std::vector<t_location> &locations);
    void                buildStatusLine();
    void                extractContentType(std::string path);
    void                extractContentLength();
    void                errorPage(const HttpRequest &request);
    void                addRedirectHeaders(const std::string &path);
    virtual bool        addBody(const HttpRequest &request, const UriResult &result);
    static unsigned int getLocation(const std::string &match, const t_server &serverConfig);
    void                attachPrefix(const std::string &uri,
                                     std::string       &path,
                                     const t_server    &location,
                                     unsigned int       index);
    UriResult           processURI(const std::string &uri);
    void                deletePath(UriResult &result, const struct stat &stats);

    virtual void addHeaders(const HttpRequest &request);
    void         addCacheHeaders();
    void         addSecurityHeaders();
    void         addConnectionHeader(const HttpRequest &request);
    void         buildAllowedMethodsHeader(const HttpRequest &request);

    // Http Phrase getters
    void getReasonPhraseInfo();
    void getReasonPhraseSuccess();
    void getReasonPhraseRedir();
    void getReasonPhraseClientErr();
    void getReasonPhraseServerErr();
};
