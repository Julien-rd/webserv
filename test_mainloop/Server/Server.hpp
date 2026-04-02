#ifndef SERVER_CLASS_HPP
# define SERVER_CLASS_HPP

# include "../HttpRequest/HttpRequest.hpp"
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
	ERR_READ
};

typedef struct s_config { // TODO This just contains things we get from the config parser. REMOVE THIS
	size_t	maxClients;
	size_t	maxBodySize;
}	t_config;

class	Server {
	private:
		t_config					config; // TODO remove this later
		int							*openFds;
		int							serverSocket;
		sockaddr_in					serverSockAddr;
		int							epfd;
		std::vector<epoll_event>	requestBuf;
		int							readyEvents;
		std::string					content;
		HttpRequest					request;

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
		Server(const t_config config);
		~Server(void);

		void	initServer(void);
		void	epollWait(void);
		void	loopReadyEvents(void);
		void	parseHttpRequest(void);
		void	handleCGI(void) const;
};

#endif /* SERVER_CLASS_HPP */
