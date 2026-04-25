#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <string>
#include "HttpRequest/HttpRequest.hpp"
#include "HttpResponse/HttpResponse.hpp"
#include "../CGI/CGI.hpp"

class Client {
	private:
		int				_fd;
		HttpRequest		_request;
		HttpResponse	_response;
		size_t			_bytesRead;
		CGI				_cgi;

		int		closeConnection(); 

	public:
		Client();

		int		loop(std::string input);
		size_t	getBytesRead();
		void	handleCGI(CGI& cgi);
		void	reset();
		void	setFd(int fd);
		int		getFd() const;
};

#endif
