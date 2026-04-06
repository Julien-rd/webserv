#ifndef SERVER_MANAGER_CLASS_HPP
# define SERVER_MANAGER_CLASS_HPP

# include "../Client/HttpRequest/HttpRequest.hpp"
// # include "../Client/HttpResponse/HttpResponse.hpp"
# include "../Server/Server.hpp"
# include "../Client/client.hpp"

# include "../headers/structs/ServerConfig.hpp"

# include <vector>
# include <string>

# include <cstddef>

# include <netinet/in.h>
# include <sys/epoll.h>

enum Type {
	ERR_EPOLL_WAIT,
	ERR_SOCKET,
	ERR_ACCEPT,
	ERR_FCNTL,
	ERR_SETSOCKOPT,
	ERR_EPOLL_CREATE1,
	ERR_EPOLL_CTL,
	ERR_BIND,
	ERR_LISTEN,
	ERR_RECV,
	ERR_CLOSE
};

typedef struct s_config { // TODO This just contains things we get from the config parser. REMOVE THIS
	std::string						globalDirective;
	std::string						globalDirective2;
	std::vector<t_server_config>	serverConfigs;
}	t_config;

class	ServerManager {
	private:
		t_config					_config; // TODO remove this later
		std::vector<Server>			_servers;
		// int							*openFds;
		Client						_clients[1024]; // maximum clients is in conf file
		int							serverSocket;
		sockaddr_in					serverSockAddr;
		int							epfd;
		std::vector<epoll_event>	requestBuf;
		int							readyEvents;
		// std::string					content; prob not needed, please check
		HttpRequest					request;

		void	validateConfig(void) const;
		void	validateServerConfig(const t_server_config config) const;
		void	initServers(void);
		void	initServerSocket(void);
		void	createEpoll(void);

		void	setServerSockAddr(void);
		void	addSocketToEpfd(int socketFd);
		void	bindAndListen(void);
		int		error_msg(Type type);

		void	setToNonBlocking(int socketFd);

		void	handleServerEvent(void);
		void	handleClientEvent(const int clientFd);

	public:
		ServerManager(const t_config config);
		~ServerManager(void);

		void	init(void);
		void	epollWait(void);
		void	loopReadyEvents(void);
		void	handleCGI(void) const;
};

#endif /* SERVER_MANAGER_CLASS_HPP */
