#pragma once
#include <cstdio>
#include <map>
#include <string>
#include <vector>

enum state {
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

enum chunkedBodyState { BYTES, LINE, _EOF };

typedef struct s_uri {
  std::string path;      //  "/python.py"
  std::string pathInfo;  //  "/extra/info"
  std::string query;     //  "key=value"
  std::string extension; //  ".py"
} t_uri;

// enum method{
//     GET,
//     HEAD, apparently: The methods GET and HEAD MUST be supported by all
//     general-purpose servers POST, DELETE,
// }; maybe better performance

class HttpRequest {

public:
  int  parseHttpRequest(std::string& recvBuffer, size_t bytes_read);
  int  parseURIContent(void);
  void print();
  HttpRequest(size_t client_max_body_size);
  HttpRequest(const HttpRequest& obj);
  HttpRequest();
  const HttpRequest& operator=(const HttpRequest& obj);
  int                getStatusCode() const;
  std::string        getMethod() const;
  bool               parsingDone();
  void               reset();
  size_t             getBytesRead() const;
  std::vector<char>  getBody() const;

  // these have to be private, please implement getters in script functions
  std::string                        _method;
  std::string                        _uri;
  std::map<std::string, std::string> _headers;
  state                              _currentState;
  size_t                             _contentLength;
  t_uri                              _uriData;

  std::string      _buffer;
  chunkedBodyState _chunkedBodyState;
  size_t           _bytesNeeded;

  // getters
  std::string getURI() const;

private:
  bool validateURIPath(std::string& path);

  bool validMethod();
  bool validUri();
  bool validHttpsVersion();
  bool hasHostHeader();
  bool hasContentLength();
  void isChunked();
  bool validNewLine(std::string& recvBuffer);
  bool containsWhiteSpaces();

  int parseHeaders(std::string& recvBuffer);
  int parseRequestLine(std::string& recvBuffer);

  int  bodyMode(std::string recvBuffer);
  void parseBody(std::string recvBuffer);
  void parseChunkedBody(std::string recvBuffer);
  bool saveData();
  void checkBodyHeaders();
  bool validateMandatoryHeaders();

  void trim();
  void findSeperator(std::string& recvBuffer, char seperator, size_t& pos,
                     size_t& max_pos);
  void exctractContent(std::string& recvBuffer, size_t pos);
  bool brokenSyntax(size_t pos, size_t max_pos);
  void addHeader();

  std::string _httpVersion;
  std::string _fieldName;
  std::string _fieldValue;
  size_t _bytesRead; // FIX: this this needs a better name what bytes, does it
                     // accumulate
  int  _statusCode;
  bool _parsingDone;
  // size_t _bytes_read implent bytes_read!!! erase is to inefficient;

  // containers
  std::vector<char> _body;

  // from .conf file
  size_t _client_max_body_size;
};
