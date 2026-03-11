#include "HttpRequest.hpp"
#include <string>
#include <iostream>

int main(){
    HttpRequest request;
    // if(argc != 1)
    //     return 1;
    // std::string content1(argv[1]);
    // std::string content = 
    // "GET /index.html HTTP/1.1\r\n"
    // "Host:localhost:8080\r\n"
    // "User-Agent:    SuperBrowser/1.0\r\n"
    // "Accept:\ttext/html\r\n"
    // "Connection: keep-alive  \r\n"
    // "X-Empty-Header:\r\n"
    // "Accept-Language: de\r\n"
    // "Connection: keep-alive  \r\n"
    // "Accept-Language: en\r\n"
    // "\r\n";
    
    std::string content = 
    "GET /index.html HTTP/1.1\r\n"
    "Host:localhost:8080\r\n"
    "User-Agent:    SuperBrowser/1.0\r\n"
    "Accept:\ttext/html\r\n"
    "Connection: keep-alive  \r\n"
    "X-Empty-Header:\r\n"
    "Accept-Language: de\r\n"
    "Connection: keep-alive  \r\n"
    "Accept-Language: en\r\n"
    "Accept-Language            : en\r\n"
    "\r\n";

    int exit_code = request.parseHttpRequest(content);
    if(exit_code == 1){
        std::cout << "first line \n";
        return 1;
    }
    if(exit_code == 2){
        std::cout << "second line \n";
        return 1;
    }
    std::cout << "success \n";
    request.print();
}