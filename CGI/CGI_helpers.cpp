#include "CGI.hpp"
#include <iostream>

std::string parsePathInfo(const std::string &_uri,
                          const std::string &scriptName) {
  const size_t pathInfoPos =
      _uri.find(scriptName) + std::string(scriptName).size();
  size_t queryStringPos = _uri.find('?');

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
        return std::string(); //ERROR: can we maybe just check if the string is empty and call it a day or can this happen
    }
  } else {
    queryStringPos = 0;
  }
  return _uri.substr(pathInfoPos, queryStringPos - pathInfoPos);
}

std::string parseQueryString(const std::string &_uri) {
  const std::string queryString;
  const size_t queryStringPos = _uri.find('?');

  if (queryStringPos == std::string::npos) {
    return "";
  }
  return _uri.substr(queryStringPos + 1);
}

// std::string	getScriptName(const std::string& _uri, const std::string& name1,
// const std::string& name2) { 	std::string	scriptName;

// 	if (_uri.compare(0, name1.size(), name1) == 0) {
// 		return name1;
// 	}
// 	else if (_uri.compare(0, name2.size(), name2) == 0) {
// 		return name2;
// 	}
// 	else {
// 		std::cout << "DEBUG: shouldn't reach here" << std::endl;
// 		throw CGI::StandardException();
// 	}

// }
