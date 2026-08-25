#include "../Utils/Macros.hpp"
#include "CGI.hpp"

#include <iostream>
#include <sstream>
void CGI::setEnv(int type, int state) {
    unsigned int entry = 0;
    _envp[entry++] = _meta.request_method.c_str();
    _envp[entry++] = _meta.query_string.c_str();
    _envp[entry++] = _meta.script_name.c_str();
    _envp[entry++] = _meta.path_info.c_str();
    // _envp[entry++] = _meta.server_name.c_str(); fix: currently not init, only init if virtual
    // hosting
    _envp[entry++] = _meta.server_port.c_str();
    _envp[entry++] = _meta.server_protocol.c_str();
    _envp[entry++] = _meta.http_cookie.c_str();
    // _envp[entry++] = _meta.remote_addr.c_str(); //fix: currently no initialized

    if (type == PHP) {
        _envp[entry++] = "REDIRECT_STATUS=1";
        _envp[entry++] = _meta.script_filename.c_str();
    }
    if (state == METHOD_GET)
        _envp[entry++] = _meta.query_string.c_str();
    if (state == METHOD_POST) {
        _envp[entry++] = _meta.content_length.c_str();
        _envp[entry++] = _meta.content_type.c_str();
    }
    _envp[entry] = NULL;
}

std::string parsePathInfo(const std::string &_uri, const std::string &scriptName) {
    const size_t pathInfoPos = _uri.find(scriptName) + std::string(scriptName).size();
    size_t       queryStringPos = _uri.find('?');

    // // This is commented out because it should already be validated and
    // confirmed to be one of the scripts (if we implement that) in
    // valduateRequest() if (pathInfoPos == std::string::npos) { 	std::cerr <<
    // "DEBUG: maybe unknown script path in parsePathInfo(). Avoid reaching here
    // in execution" << std::endl; 	throw CGI::StandardException();
    // }
    if (queryStringPos != std::string::npos) {
        if (queryStringPos < pathInfoPos) {
            std::cerr << "DEBUG: found '?' before PATH_INFO in parsePathInfo(). "
                         "Avoid reaching here in execution"
                      << std::endl;
            return std::string();  // ERROR: can we maybe just check if the string is empty and call
                                   // it a day or can this happen
        }
    } else {
        queryStringPos = 0;
    }
    return _uri.substr(pathInfoPos, queryStringPos - pathInfoPos);
}

void CGI::setMeta(std::string &field, const std::string &key, const std::string &value) {
    field = key + "=" + value;
}

void CGI::initMeta(int type) {

    // optionals:
    //  _meta.auth_type = ""; // RFC 3875 - 4.1.1 // Implement?
    //  setMeta(_meta.remote_addr, "REMOTE_ADDR=", ""); //fix: actually useful sometimes question is
    //  where do we get the clients ip in what header _meta.server_name =
    //      std::string("SERVER_NAME=")
    //          .append(_request->_headers.at(
    //              "Host")); // TODO get this from result of config_parser instead

    const std::map<std::string, std::string> &headers = _request->getHeaders();
    std::stringstream                         ss;
    ss << _request->getContentLength();

    if (_request->getMethod() == "POST") {
        setMeta(_meta.content_length, "CONTENT_LENGTH", ss.str());  // fix: chunks?
    } else
        setMeta(_meta.content_length, "CONTENT_LENGTH", "0");  // fix: chunks?

    if (_request->getHeaders().find("content-type") != headers.end())
        setMeta(_meta.content_type, "CONTENT_TYPE", headers.at("content-type"));

    setMeta(_meta.path_info, "PATH_INFO", _request->getUriData().pathInfo);
    setMeta(_meta.query_string, "QUERY_STRING", _request->getQuery());
    setMeta(_meta.request_method, "REQUEST_METHOD", _request->getMethod());
    setMeta(_meta.script_name, "SCRIPT_NAME", _scriptName);
    setMeta(_meta.server_port, "SERVER_PORT", _serverConfig->port);
    setMeta(_meta.server_protocol, "SERVER_PROTOCOL", "HTTP/1.1");
    if (_request->getHeaders().find("cookie") != _request->getHeaders().end())
        setMeta(_meta.http_cookie, "HTTP_COOKIE", headers.at("cookie"));
    if (type == PHP)
        setMeta(_meta.script_filename,
                "SCRIPT_FILENAME=cgi-bin",
                _request->getUriData().path);  // TODO make this actually adaptable
}

bool CGI::setScriptAttributes(int type) {
    std::string extension;
    if (type == PYTHON)
        extension = ".py";
    else if (type == PHP)
        extension = ".php";

    for (size_t i = 0; i < _cgiConfigs->size(); i++) {
        if (_cgiConfigs->at(i).extension == extension) {
            _executable = _cgiConfigs->at(i).executablePath;
            _argv.push_back(_cgiConfigs->at(i).executablePath);
            _argv.push_back(ROOT_FOLDER + _serverConfig->locations.at(0).root +
                            _request->getUriData().path);
            return true;
        }
    }
    log(Level::WARNING, "CGI file extension not found in setScriptAttributes()");
    return false;
}

bool CGI::initScript(int type) {
    initMeta(type);
    if (!setScriptAttributes(type))
        return false;
    if (_request->getMethod() == "GET") {
        setEnv(type, METHOD_GET);
    } else if (_request->getMethod() == "POST") {
        setEnv(type, METHOD_POST);
    } else {
        log(Level::WARNING, "Unknown method in CGI");
        return false;
    }
    return true;
}

std::string parseQueryString(const std::string &_uri) {
    const std::string queryString;
    const size_t      queryStringPos = _uri.find('?');

    if (queryStringPos == std::string::npos) {
        return "";
    }
    return _uri.substr(queryStringPos + 1);
}

