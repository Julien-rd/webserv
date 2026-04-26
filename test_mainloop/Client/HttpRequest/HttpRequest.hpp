#ifndef HTTPREQUESTHPP
#define HTTPREQUESTHPP

#include <cstdio>
#include <string>
#include <map>
#include <vector>

enum state{
    METHOD,
    URI,
    HTTP_VERSION,
    CR,
    FIELD_NAME,
    FIELD_VALUE,
    EOH,
    BODY,
    BODY_CHUNKED
};

// enum method{
//     GET,
//     HEAD, apparently: The methods GET and HEAD MUST be supported by all general-purpose servers
//     POST,
//     DELETE,
// }; maybe better performance


class HttpRequest{

    public:
    int parseHttpRequest(std::string request_content, size_t bytes_read);
    void print();
    HttpRequest(size_t client_max_body_size);
    HttpRequest();
    const HttpRequest& operator=(const HttpRequest& obj);
    int getStatusCode() const;
    bool parsingDone();
    void reset();
    size_t getBytesRead() const;
    std::vector<char> getBody() const;

    // these have to be private, please implement getters in script functions
    std::string _method;
    std::string _uri;
    std::map<std::string, std::string> _headers;
    state _currentState;
    size_t _contentLength;

    //getters
    std::string getURI() const;
    
    private:
    bool validMethod();
    bool validUri();
    bool validHttpsVersion();
    bool hasHostHeader();
    bool hasContentLength();
    void isChunked();
    bool validNewLine(std::string request_content);
    bool containsWhiteSpaces();

    int parseHeaders(std::string &request_content);
    int parseRequestLine(std::string &request_content);
    int parse_body(std::string request_content);
    bool validateMandatoryHeaders();

    void trim();
    void findSeperator(std::string &request_content, char seperator, size_t &pos, size_t &max_pos);
    void exctractContent(std::string &request_content, size_t pos);
    bool brokenSyntax(size_t pos, size_t max_pos);
    void addHeader();

    std::string _httpVersion;
    std::string _fieldName;
    std::string _fieldValue;
    size_t _bytesRead;
    int _statusCode;
    bool _parsingDone;
    // size_t _bytes_read implent bytes_read!!! erase is to inefficient;

    //containers
    std::vector<char> _body;

    //from .conf file
    size_t _client_max_body_size;
};

#endif
