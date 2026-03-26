#ifndef CGI_HPP
# define CGI_HPP

# include "HttpRequest.hpp"
# include <string>
# include <exception>

# define FT_DEFINE_EXCEPTION(exception_name, message)\
	class exception_name: public std::exception {\
		public: const char *what(void) const throw() {\
			return message;\
		}\
	}

// # define FT_THROW(class_name, exception_name)
// 	throw class_name##_##exception_name()

typedef struct s_metaVariables {
	std::string	auth_type;
    std::string	content_length;
    std::string	content_type;
    std::string	gateway_interface;
    std::string	path_info;
    std::string	path_translated;
    std::string	query_string;
    std::string	remote_addr;
    std::string	remote_host;
    std::string	remote_ident;
    std::string	remote_user;
    std::string	request_method;
    std::string	script_name;
    std::string	server_name;
    std::string	server_port;
    std::string	server_protocol;
    std::string	server_software;
}	t_metaVariables;

class	CGI {
	private:
		t_metaVariables	metaVariables;
		char			*path;

		pid_t			pid;
		int				pipefd[2];
		char			*argv[2];
		const char		**envp;

		void	setGETVariables(const HttpRequest& request);
		void	setPOSTVariables(const HttpRequest& request);
		void	setMetaVariables(const HttpRequest& request);

	public:
		CGI(void);
		~CGI(void);

		pid_t	getPid(void) const;
		void	setPid(pid_t pid);
		void	validateRequest(const HttpRequest& request) const;
		void	initCGI(const char **newEnvp, const HttpRequest& request);
		void	spawnProcess(void);
		void	pipeIO(void);
		void	redirectIO(void);
		void	execute(void);

		FT_DEFINE_EXCEPTION(StandardException, "ERROR: Standard Exception");
		FT_DEFINE_EXCEPTION(WaitException, "EXCEPTION CAUGHT IN PARENT: waiting");
};

#endif /* CGI_HPP */
