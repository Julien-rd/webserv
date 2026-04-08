#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <string>
#include "HttpRequest/HttpRequest.hpp"
#include "HttpResponse/HttpResponse.hpp"

class Client {
	private:
		int				_fd;
		HttpRequest		_request;
		HttpResponse	_response;
		size_t			_bytesRead;

		int		closeConnection(); 

	public:
		Client();

		int		loop(std::string input);
		size_t	getBytesRead();
		void	reset();
		void	setFd(int fd);
		int		getFd() const;
};

#endif
