#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <string>
#include "HttpRequest/HttpRequest.hpp"
#include "HttpResponse/HttpResponse.hpp"

class Client {
    public:
    int loop(std::string input);
    size_t getBytesRead();
    void reset();
    Client();
    void setFd(int fd);
    int getFd() const;
    private:
    int closeConnection();
    HttpRequest _request;
    HttpResponse _response;
    size_t _bytesRead;
    int _fd;
};

#endif
