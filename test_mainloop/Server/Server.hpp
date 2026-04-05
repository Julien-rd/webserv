#ifndef SERVER_CLASS_HPP
# define SERVER_CLASS_HPP

# include "../Client/HttpRequest/HttpRequest.hpp"
// # include "../Client/HttpResponse/HttpResponse.hpp"
# include "../Client/client.hpp" // FIXME fix file name "client.hpp" > "Client.hpp"

# include "../headers/structs/ServerConfig.hpp"

# include <string>
# include <vector>
# include <string>

# include <cstddef>

# include <netinet/in.h>
# include <sys/epoll.h>

class Server {
	private:
		const t_server_config	_config;
		std::string				_name;
		// int					serverSocket;
		// sockaddr_in			serverSockAddr;

	public:
		Server(const t_server_config& conf);
		~Server(void);
		Server(const Server& obj);
		Server&	operator=(const Server& obj);

		void	init(void);

		std::string	getName(void) const;
};

#endif /* SERVER_CLASS_HPP */
