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

# define CLIENTS 1024 // FIXME Remove hardcode
# define SERVER_LIMIT 1024 // FIXME

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
		// Client					_clients[CLIENTS]; // maximum clients is in conf file // this is now moved to Server class
		// std::set<int>			_serversFds;
		std::map<int, Server>		_servers; // Key: the fd of the server. Value: the server
		std::map<int, IntSet>		_ServersClientsFds; // Key: the fd of the server. Value: all of its current clients
		std::vector<epoll_event>	_requestBuf;
		int							_readyEvents;

		// std::string					content; prob not needed, please check
		HttpRequest					request; // TODO This is unhandled in multi-server structure

		void	validateConfig(void) const;
		void	validateServerConfig(const t_server_config config) const;
		void	createEpoll(void);
		void	startServers(void);

		int		error_msg(ErrorType type);

	public:
		ServerManager(const t_config config);
		~ServerManager(void);

		void	init(void);
		void	epollWait(void);
		void	loopReadyEvents(void);
		void	handleCGI(void) const;
};

#endif /* SERVER_MANAGER_CLASS_HPP */
