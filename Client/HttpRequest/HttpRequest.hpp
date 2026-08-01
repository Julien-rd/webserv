#pragma once
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <map>
#include <string>
#include <vector>

enum state { METHOD, URI, HTTP_VERSION, CR, FIELD_NAME, FIELD_VALUE, EOH, BODY, BODY_CHUNKED };

enum chunkedBodyState { BYTES, LINE, _EOF };

typedef struct s_uri {
    std::string path;       //  "/python.py"
    std::string pathInfo;   //  "/extra/info"
    std::string query;      //  "key=value"
    std::string extension;  //  ".py"
} t_uri;

// enum method{
//     GET,
//     HEAD, apparently: The methods GET and HEAD MUST be supported by all
//     general-purpose servers POST, DELETE,
// }; maybe better performance

class HttpRequest {

  public:
    HttpRequest(size_t clientMaxBody);
    HttpRequest(const HttpRequest &obj);
    HttpRequest();
    const HttpRequest &operator=(const HttpRequest &obj);

    int  parseHttpRequest(std::string &recvBuffer, size_t bytes_read);
    int  parseURIContent(void);
    bool parsingDone();

    void print();
    void init(unsigned int clientMaxBody);
    void reset();

    // getters
    int                                       getStatusCode() const;
    size_t                                    getBytesRead() const;
    const std::vector<char>                  &getBody() const;
    const std::string                        &getMethod() const;
    const std::string                        &getUri() const;
    const std::map<std::string, std::string> &getHeaders() const;
    state                                     getCurrentState() const;
    size_t                                    getContentLength() const;
    const t_uri                              &getUriData() const;
    const std::string                        &getBuffer() const;
    chunkedBodyState                          getChunkedBodyState() const;
    size_t                                    getBytesNeeded() const;

    // setters
    void setStatusCode(int status);

  private:
    std::string                        _method;
    std::string                        _uri;
    std::map<std::string, std::string> _headers;
    state                              _currentState;
    size_t                             _contentLength;
    t_uri                              _uriData;
    std::string                        _buffer;
    chunkedBodyState                   _chunkedBodyState;
    size_t                             _bytesNeeded;
    std::string                        _httpVersion;
    std::string                        _fieldName;
    std::string                        _fieldValue;
    size_t                             _bytesRead;  // FIX: this this needs a better name workflow
    int                                _statusCode;
    bool                               _parsingDone;
    std::vector<char>                  _body;
    size_t                             _clientMaxBody;

    // size_t _bytes_read implent bytes_read!!! erase is to inefficient

    bool validateURIPath(std::string &path);
    bool validMethod();
    bool validUri();
    bool validHttpsVersion();
    bool hasHostHeader();
    bool hasContentLength();
    bool isChunked();
    bool validNewLine(std::string &recvBuffer);
    bool containsWhiteSpaces();
    int  parseHeaders(std::string &recvBuffer);
    int  parseRequestLine(std::string &recvBuffer);
    int  bodyMode(std::string recvBuffer);
    void parseBody(std::string recvBuffer);
    int  parseChunkedBody(std::string recvBuffer);
    bool saveData();
    bool validateMandatoryHeaders();
    void trim();
    void findSeperator(std::string &recvBuffer, char seperator, size_t &pos, size_t &max_pos);
    void exctractContent(std::string &recvBuffer, size_t pos);
    bool brokenSyntax(size_t pos, size_t max_pos);
    void addHeader();
};
