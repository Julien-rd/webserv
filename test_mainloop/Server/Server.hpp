#ifndef SERVER_CLASS_HPP
# define SERVER_CLASS_HPP

# include "../Client/HttpRequest/HttpRequest.hpp"
// # include "../Client/HttpResponse/HttpResponse.hpp"
# include "../Client/client.hpp" // FIXME fix file name "client.hpp" > "Client.hpp"

# include "../headers/structs/ServerConfig.hpp"
# include "../headers/structs/ErrorType.hpp"

# include <string>
# include <set>

# include <netinet/in.h>
# include <sys/epoll.h>

class Server {
	private:
		typedef std::set<int>	IntSet;
		const t_server_config	_config;
		std::string				_name;

		Client					_clients[1024]; // maximum clients is in conf file
		int						_serverSocket;
		sockaddr_in				_serverSockAddr;
		int						_epfd;

		void	initServerSocket(void);
		void	setServerSockAddr(void);
		void	addSocketToEpoll(int socketFd);
		void	bindAndListen(void);
		void	setToNonBlocking(int socketFd);

	public:
		Server(const t_server_config& conf, int epfd);
		~Server(void);
		Server(const Server& obj);
		Server&	operator=(const Server& obj);

		void	handleServerEvent(std::map<int, IntSet>& ManagersClients);
		void	handleClientEvent(int clientFd);

		int		start(void);

		std::string	getName(void) const;

		int		error_msg(ErrorType type);
};

#endif /* SERVER_CLASS_HPP */
