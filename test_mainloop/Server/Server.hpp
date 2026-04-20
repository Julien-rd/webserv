#ifndef SERVER_CLASS_HPP
# define SERVER_CLASS_HPP

# include "../Client/client.hpp" // FIXME fix file name "client.hpp" > "Client.hpp"
# include "../ParseConfig/Parser.hpp"
# include "../headers/structs/ErrorType.hpp"

# include <string>
# include <set>

# include <netinet/in.h>
# include <sys/epoll.h>

#define BUFFER_SIZE 4096

enum e_operation {
	ADD,
	REMOVE
};

class Server {
	private:
		typedef std::set<int>			IntSet;

		const   t_server&				_config;
		int                         _sid; //server identifier
		int							_serverSocket;
		sockaddr_in					_serverSockAddr;
		int							_epfd;
		std::map<int, IntSet>&		_clientsMap;
		std::map<int, Client>&		_clients;

		void		initServerSocket(void);
		void		setServerSockAddr(void);
		void		addSocketToEpoll(int socketFd);
		void		bindAndListen(void);
		void		setToNonBlocking(int socketFd);

		void		closeConnection(int clientFd);
		void		updateClientsMap(enum e_operation opeartion, const int clientFd);

	public:
		Server(const t_config& conf, int epfd, std::map<int, IntSet>& _clientsMap, std::map<int, Client>& clients, int sid);
		Server(const Server& obj);
		~Server(void);

		void		checkClientCap(void);
		void		handleServerEvent(void);
		void		handleClientEvent(int clientFd);

		int			start(void);
		void		closeClientFds(void);

		int	getIdentifier(void) const;

		int			error_msg(ErrorType type);
};

#endif /* SERVER_CLASS_HPP */
