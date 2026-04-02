#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <string>
#include "HttpRequest/HttpRequest.hpp"
#include "HttpResponse/HttpResponse.hpp"

class Client {
    public:
    void loop(std::string input, int fd);
    size_t getBytesRead();
    Client();
    private:
    HttpRequest _request;
    HttpResponse _response;
    size_t _bytesRead;
    int _fd;
};

#endif