#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <string>
#include "HttpRequest/HttpRequest.hpp"
#include "HttpResponse/HttpResponse.hpp"
#include "../CGI/CGI.hpp"

class Client {
	private:
		int				_fd;
		int				_epfd;
		HttpRequest		_request;
		HttpResponse	_response;
		size_t			_bytesRead;

		int		closeConnection(); 

	public:
		Client();
		Client(int epfd);
		Client(const Client& obj);
		const Client&	operator=(const Client& obj);

		CGI				_cgi;
		int		loop(std::string input);
		size_t	getBytesRead();
		void	handleCGI(CGI& cgi);
		void	reset();
		void	setFd(int fd);
		int		getFd() const;
};

#endif
