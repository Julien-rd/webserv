#ifndef HTTPREQUESTHPP
#define HTTPREQUESTHPP

#include <string>
#include <map>

enum state{
    METHOD,
    // SP,
    URI,
    HTTP_VERSION,
    // CRLF,
    FIELD_NAME,
    FIELD_VALUE,
    STATE_BODY,
    STATE_BODY_CHUNKED
};

class HttpRequest{

    public:
    int parseHttpRequest(std::string request_content);
    void print();
    HttpRequest();
    
    private:
    bool validMethod();
    bool validUri();
    bool validHttpsVersion();
    bool hasHostHeader();
    bool hasContentLength();
    void isChunked();

    int parseHeaders(std::string &request_content);
    int parseRequestLine(std::string &request_content);
    bool validateMandatoryHeaders();

    void trim();
    void findSeperator(std::string &request_content, char seperator, size_t &pos, size_t &max_pos);
    void exctractContent(std::string &request_content, size_t pos);
    bool brokenSyntax(size_t pos, size_t max_pos);
    void addHeader();

    state _currentState;
    std::string _method;
    std::string _uri;
    std::string _httpVersion;
    std::map<std::string, std::string> _headers;
    std::string _fieldName;
    std::string _fieldValue;
    size_t _contentLength;
};

#endif