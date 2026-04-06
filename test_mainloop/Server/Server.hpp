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

		Client					_clients[1024]; // maximum clients is in conf file
		int						_serverSocket;
		sockaddr_in				_serverSockAddr;
		int						epfd;
		// std::vector<epoll_event>	requestBuf;
		int						readyEvents;
		// std::string			content; prob not needed, please check
		HttpRequest				request;

	public:
		Server(const t_server_config& conf);
		~Server(void);
		Server(const Server& obj);
		Server&	operator=(const Server& obj);

		// TODO implement
		void	initServerSocket(void);
		void	createEpoll(void);

		void	setServerSockAddr(void);
		void	addSocketToEpoll(int socketFd);
		void	bindAndListen(void);
		int		error_msg(Type type);

		void	setToNonBlocking(int socketFd);
		// TODO till here

		void	init(void);

		std::string	getName(void) const;
};

#endif /* SERVER_CLASS_HPP */
