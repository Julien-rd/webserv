#include "HttpRequest.hpp"
#include <string>
#include <iostream>

HttpRequest::HttpRequest(): _currentState(METHOD), _contentLength(0){
}

void HttpRequest::print(){
    std::cout << "---------------------RequestLine---------------------\n";
    std::cout << "Method: " << _method << "\n" << "URI: " << _uri << "\n" << "HTTP_VERSION: " << _httpVersion << "\n";
    std::cout << "\n---------------------Headers---------------------\n";
    for(std::map<std::string, std::string>::iterator it = _headers.begin(); it != _headers.end(); ++it){
        std::cout << "Fieldname: [" << it->first << "], Fieldvalue: [" << it->second << "]" <<  "\n";
    }
}

int HttpRequest::parseRequestLine(std::string &request_content){
    size_t pos;
    size_t max_pos;
    switch (_currentState){
        case METHOD:
            findSeperator(request_content, ' ', pos, max_pos);
            if(brokenSyntax(pos, max_pos))
                return 1;
            if(pos == std::string::npos){
                _method += request_content;
                break ;
            }
            exctractContent(request_content, pos);
            if(!validMethod())
                return 1; // 501 not implemented, 404 kauderwelsch
            _currentState = URI;
        case URI:
            findSeperator(request_content, ' ', pos, max_pos);
            if(brokenSyntax(pos, max_pos))
                return 1;
            if(pos == std::string::npos){
                _uri += request_content;
                break ;
            }
            exctractContent(request_content, pos);
            if(!validUri())
                return 1;
            _currentState = HTTP_VERSION;
        case HTTP_VERSION:
            pos = request_content.find("\r\n");
            if(pos == std::string::npos){
                _httpVersion += request_content;
                break ;
            }
            exctractContent(request_content, pos);
            if(!validHttpsVersion())
                return 1;
            _currentState = FIELD_NAME;
        default:
            ;
    }
    return 0;
}


int HttpRequest::parseHeaders(std::string &request_content){
    size_t pos;
    size_t max_pos;
    for(std::string::iterator it = request_content.begin(); it != request_content.end(); ++it){
        switch (_currentState){
            case FIELD_NAME:
                findSeperator(request_content, ':', pos, max_pos);
                if(pos == std::string::npos && max_pos != std::string::npos){
                    if(max_pos != 0){
                        std::cerr << "Invalid header!\n";
                        return 1;
                    }
                    _currentState = STATE_BODY;
                    return 0;
                }
                if(pos == std::string::npos){
                    _fieldName += request_content;
                    return 0;
                }
                exctractContent(request_content, pos);
                trim();
                if(_fieldName.empty()){
                    std::cerr << "fieldname cannot be empty\n";
                    return 1;
                }
                _currentState = FIELD_VALUE;
            case FIELD_VALUE:
                pos = request_content.find("\r\n");
                if(pos == std::string::npos){
                    _fieldValue += request_content;
                    return 0;
                }
                exctractContent(request_content, pos);
                trim();
                addHeader();
                _currentState = FIELD_NAME;
            default:
                ;
        }
    }
    return 1;
}

int HttpRequest::parseHttpRequest(std::string request_content){
    if (parseRequestLine(request_content) == 1)
        return 1;
    if (parseHeaders(request_content) == 1)
        return 1;
    if(validateMandatoryHeaders() == false)
        return 1;
    //parse body;
    return 0;
}