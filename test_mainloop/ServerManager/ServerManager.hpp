#ifndef SERVER_MANAGER_CLASS_HPP
# define SERVER_MANAGER_CLASS_HPP

// # include "../Client/HttpResponse/HttpResponse.hpp"
# include "../Server/Server.hpp"

# include "../headers/structs/ServerConfig.hpp"
# include "../headers/structs/ErrorType.hpp"

# include <vector>
# include <set>
# include <map>

# include <netinet/in.h>
# include <sys/epoll.h>

# define CLIENT_LIMIT 2048
# define MAX_EVENTS 1024

typedef struct s_config {
	size_t							max_clients;
	std::vector<t_server_config>	serverConfigs;
}	t_config;

class	ServerManager {
	private:
		typedef std::set<int>		IntSet;

		t_config					_config;
		int							_epfd;
		std::map<int, Server>		_serversMap; // Key: the fd of the server. Value: the server
		std::map<int, IntSet>		_clientsMap; // Key: the fd of the server. Value: all of its current clients
		std::vector<epoll_event>	_requestBuf;
		int							_readyEvents;

		void	validateServerConfig(const t_server_config config) const;
		void	validateConfig(void) const;
		void	createEpoll(void);
		void	addServerToMaps(int serverSocket, Server& server);
		void	startServers(void);

		int		error_msg(ErrorType type);

	public:
		ServerManager(const t_config config);
		~ServerManager(void);

		void	init(void);
		void	epollWait(void);
		void	loopReadyEvents(void);
		int		matchClientToServer(int fd);
};

#endif /* SERVER_MANAGER_CLASS_HPP */
