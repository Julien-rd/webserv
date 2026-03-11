#ifndef HTTPRESPONSEHPP
#define HTTPRESPONSEHPP

#include <string>


class HttpResponse{
    public:
    void build(const std::string content_path);

    private:
    size_t content_size;
    const std::string http_type = "HTTP/1.1";
    std::string http_header;
};

#endif