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
		t_server_config			_config;
		std::string				_name;
		Client					_clients[1024]; // TODO maximum clients is in conf file
		int						_serverSocket;
		sockaddr_in				_serverSockAddr;
		int						_epfd;
		std::map<int, IntSet>&	_clientsMap;

		void		initServerSocket(void);
		void		setServerSockAddr(void);
		void		addSocketToEpoll(int socketFd);
		void		bindAndListen(void);
		void		setToNonBlocking(int socketFd);

	public:
		// Server(void);
		// Server(int epfd);
		Server(const t_server_config& conf, int epfd, std::map<int, IntSet>& _clientsMap);
		Server(const Server& obj);
		~Server(void);
		// Server&	operator=(const Server& obj);

		void		handleServerEvent(void);
		void		handleClientEvent(int clientFd);
		void		removeClientFd(const int clientFd);

		int			start(void);
		void		closeClientFds(void) const;

		std::string	getName(void) const;
		// int			getServerSocket(void) const;
		// void		setConfig(const t_server_config& config);
		// void		setEpfd(int epfd);

		int			error_msg(ErrorType type);
};

#endif /* SERVER_CLASS_HPP */
