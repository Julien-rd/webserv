#include "HttpRequest.hpp"
#include <iostream>
#include <sstream>

void HttpRequest::addHeader(){
    if(_headers.count(_fieldName) > 0){
        _headers[_fieldName] += ", " + _fieldValue;
    }
    else
        _headers[_fieldName] = _fieldValue;
    _fieldName.clear();
    _fieldValue.clear();
}

void HttpRequest::trim(){
    size_t pos;
    switch (_currentState){
        case FIELD_NAME:
            pos = _fieldName.find_last_not_of(" \t\r\n");
            if(pos != std::string::npos)
                _fieldName.erase(pos + 1);
            break ;
        case FIELD_VALUE:
            pos = _fieldValue.find_first_not_of(" \t\r\n");
            if(pos != std::string::npos)
                _fieldValue.erase(0, pos);
            pos = _fieldValue.find_last_not_of(" \t\r\n");
            if(pos != std::string::npos)
                _fieldValue.erase(pos + 1);
        default:
            ;
    }
}

void HttpRequest::exctractContent(std::string &request_content, size_t pos){
    size_t skip = 1;
    switch (_currentState){
        case METHOD:
            _method += request_content.substr(0, pos);
            break ;
        case URI:
            _uri += request_content.substr(0, pos);
            break ;
        case FIELD_NAME:
            _fieldName += request_content.substr(0, pos);
            break ;
        case FIELD_VALUE:
            _fieldValue += request_content.substr(0, pos);
            skip = 2;
            break ;
        case HTTP_VERSION:
            _httpVersion += request_content.substr(0, pos);
            skip = 2;
        default:
            ;
    }
    request_content.erase(0, pos + skip);
}

bool HttpRequest::brokenSyntax(size_t pos, size_t max_pos){
    return (pos == std::string::npos && max_pos != std::string::npos);
}

void HttpRequest::findSeperator(std::string &request_content, char seperator, size_t &pos, size_t &max_pos){
    max_pos = request_content.find("\r\n");
    pos = request_content.find(seperator);
}

bool HttpRequest::validMethod(){
    return (_method == "GET" || _method == "POST" || _method == "DELETE");
}

bool HttpRequest::validUri(){
    if(_uri.size() > 4096 || *(_uri.begin()) != '/')
        return false;
    for(std::string::iterator it = _uri.begin(); it != _uri.end(); ++it){
        if(*it < 33 || *it > 126)
            return false;
    }
    return (true);
}

bool HttpRequest::validHttpsVersion(){
    return (_httpVersion == "HTTP/1.1");
}

bool HttpRequest::hasHostHeader(){
    std::map<std::string, std::string>::iterator it = _headers.find("Host");
    if (it == _headers.end())
        return false;
    return true;
}

bool HttpRequest::hasContentLength(){
    std::map<std::string, std::string>::iterator it = _headers.find("Content-Length");
    if (it != _headers.end()){
        if(_currentState == STATE_BODY_CHUNKED){
            std::cerr << "Header has both transfer-encoding and content-length\n";
            return false;
        }
        std::stringstream ss(_headers["Content-Length"]);
        ss >> _contentLength;
        if (ss.fail()){
            std::cerr << "Invalid Content-Length\n";
            return false;
        }
    }
    return true;
}

void HttpRequest::isChunked(){
    std::map<std::string, std::string>::iterator it = _headers.find("Transfer-Encoding");
    if (it == _headers.end())
        return ;
    if(it->second == "chunked")
        _currentState = STATE_BODY_CHUNKED;
    return ;
}

bool HttpRequest::validateMandatoryHeaders(){
    isChunked();
    return (hasHostHeader() && hasContentLength());
}