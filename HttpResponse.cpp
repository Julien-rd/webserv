#include "HttpResponse.hpp"
#include <string>
#include <fstream>



void HttpResponse::build(const std::string content_path){
    std::ifstream html_file(content_path);
    http_header += http_type;
    if(!html_file)
        // error_handling();
    
}