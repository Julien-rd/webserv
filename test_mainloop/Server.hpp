#ifndef SERVER_CLASS_HPP
# define SERVER_CLASS_HPP

# include <vector>

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

typedef struct s_configParser { // TODO This just contains things we get from the config parser. REMOVE THIS
	size_t	maxClients;
}	t_configParser;

class	Server {
	private:
		size_t						maxClients;
		int							*openFds;
		int							serverSocket;
		sockaddr_in					serverSockAddr;
		int							epfd;
		std::vector<epoll_event>	requestBuf;
		int							readyEvents;

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
		Server(const int maxClients);
		~Server(void);

		void	initServer(void);
		void	epollWait(void);
		void	loopReadyEvents(void);
};

#endif /* SERVER_CLASS_HPP */
