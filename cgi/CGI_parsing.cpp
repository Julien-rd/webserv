#include "CGI.hpp"
#include <iostream>

#define SCRIPT_NAME "/cgi-bin/script.py"

std::string	parsePathInfo(const std::string& _uri) {
	const size_t	pathInfoPos = _uri.find(SCRIPT_NAME) + std::string(SCRIPT_NAME).size();
	size_t			queryStringPos = _uri.find('?');

	// // This is commented out because it should already be validated and confirmed to be one of the scripts (if we implement that) in valduateRequest()
	// if (pathInfoPos == std::string::npos) {
	// 	std::cerr << "DEBUG: maybe unknown script path in parsePathInfo(). Avoid reaching here in execution" << std::endl;
	// 	throw CGI::StandardException();
	// }
	if (queryStringPos != std::string::npos) {
		if (queryStringPos < pathInfoPos) {
			std::cout << "DEBUG: found '?' before PATH_INFO in parsePathInfo(). Avoid reaching here in execution" << std::endl;
			throw CGI::StandardException();
		}
	}
	else {
		queryStringPos = 0;
	}
	return _uri.substr(pathInfoPos, queryStringPos - pathInfoPos);
}

std::string	parseQueryString(const std::string& _uri) {
	const std::string	queryString;
	const size_t		queryStringPos = _uri.find('?');

	if (queryStringPos == std::string::npos) {
		return "";
	}
	return _uri.substr(queryStringPos + 1);
}
