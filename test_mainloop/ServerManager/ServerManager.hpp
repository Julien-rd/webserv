#ifndef SERVER_MANAGER_CLASS_HPP
# define SERVER_MANAGER_CLASS_HPP

# include "../Client/HttpRequest/HttpRequest.hpp"
// # include "../Client/HttpResponse/HttpResponse.hpp"
# include "../Server/Server.hpp"
# include "../Client/client.hpp"

# include "../headers/structs/ServerConfig.hpp"
# include "../headers/structs/ErrorType.hpp"

# include <vector>
# include <string>
# include <set>
# include <map>

# include <netinet/in.h>
# include <sys/epoll.h>

# define TOTAL_CLIENT_LIMIT 40
# define SERVER_CLIENT_LIMIT 10
# define SERVER_LIMIT 10

typedef struct s_config { // TODO This just contains things we get from the config parser. REMOVE THIS
	std::string						globalDirective;
	std::string						globalDirective2;
	std::vector<t_server_config>	serverConfigs;
}	t_config;

class	ServerManager {
	private:
		typedef std::set<int>		IntSet;

		t_config					_config; // TODO remove this later
		int							_epfd;
		std::map<int, Server>		_serversMap; // Key: the fd of the server. Value: the server
		std::map<int, IntSet>		_clientsMap; // Key: the fd of the server. Value: all of its current clients
		std::vector<epoll_event>	_requestBuf;
		int							_maxEvents;
		int							_readyEvents;

		// std::string					content; prob not needed, please check
		HttpRequest					request; // TODO This is unhandled in multi-server structure

		void	validateConfig(void) const;
		void	validateServerConfig(const t_server_config config) const;
		void	initRequestBuf(void);
		void	createEpoll(void);
		void	startServers(void);
		void	setServerMaps(int serverSocket, Server& server);
		int		error_msg(ErrorType type);

	public:
		ServerManager(const t_config config);
		~ServerManager(void);

		void	init(void);
		void	epollWait(void);
		void	loopReadyEvents(void);
		int		matchClientToServer(int fd);
		void	handleCGI(void) const; // TODO This is unhandled in multi-server structure
};

#endif /* SERVER_MANAGER_CLASS_HPP */
