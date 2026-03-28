#ifndef SERVER_CLASS_HPP
# define SERVER_CLASS_HPP

# include <cstddef>
#include <netinet/in.h>

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

class	Server {
	private:
		size_t		maxClients;
		int			*openFds;
		sockaddr_in	serverSockAddr;
		int			epfd;

		void	initSocket(void);
		void	createEpoll(void);
		
		void	setServerSockAddr(void);
		void	addSocketToEpfd(int socketFd);
		void	bindAndListen(void);
		int		error_msg(Type type);

	public:
		int			serverSocket;

		Server(const int max_clients);
		~Server(void);

		void	initServer(void);
};


#endif /* SERVER_CLASS_HPP */
