#ifndef SERVER_CLASS_HPP
# define SERVER_CLASS_HPP

# include "../Client/client.hpp" // FIXME fix file name "client.hpp" > "Client.hpp"
# include "../ParseConfig/Parser.hpp"
# include "../headers/structs/ErrorType.hpp"
# include "../headers/structs/ServerStructs.hpp"

# include <string>
# include <set>

# include <netinet/in.h>
# include <sys/epoll.h>
#include <fcntl.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

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
		t_serverContext             _context;
		int							_serverSocket;
		addrinfo					*_addrInfo;
		std::map<int, IntSet>&		_clientsMap;
		std::map<int, int>&			_clientToServerMap;
		std::map<int, Client>&		_clients;

		void		initServerSocket(void);
		void		setServerSockAddr(void);
		void		addSocketToEpoll(int socketFd);
		void		bindAndListen(void);
		void		setToNonBlocking(int socketFd);

		void		closeConnection(int clientFd);
		void		updateClientsMap(enum e_operation opeartion, const int clientFd);

	public:
		Server(const t_server& conf, std::map<int, IntSet>& _clientsMap,
			std::map<int, Client>& clients, std::map<int, int>& _clientToServerMap, t_serverContext &context, int sid);
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
