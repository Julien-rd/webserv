#ifndef CGI_HPP
# define CGI_HPP

# include "../Client/HttpRequest/HttpRequest.hpp"
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

class	Client;

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
	std::string	script_filename;
}	t_metaVariables;

class	CGI {
	private:
		const HttpRequest&	request;
		std::string			scriptName;
		t_metaVariables		meta;
		int					pid;
		int					epfd;
		Client&				client;

		const char			*executable;
		char				*argv[3];
		const char			*envp[20];

		const std::string	pythonScriptName;
		const std::string	phpScriptName;

		void	initPythonScript(void);
		void	initMetaPython(void);
		void	setScriptAttributesPython(void);
		void	setGETVariablesPython(void);
		void	setPOSTVariablesPython(void);

		void	initPhpScript(void);
		void	initMetaPhp(void);
		void	setScriptAttributesPhp(void);
		void	setGETVariablesPhp(void);
		void	setPOSTVariablesPhp(void);

		void	execute(void);

	public:
		CGI(const HttpRequest& request, Client& client, int epfd);
		~CGI(void);
		const CGI&	operator=(const CGI& obj);

		// pid_t	getPid(void) const;
		// void	setPid(pid_t pid);

		int					pipefd[2];
		bool	validateRequest(void) const;
		void	initCGI(void);
		void	pipeIO(void);
		void	redirectIO(void);
		void	spawnProcess(void);
		void	addPipeToEpoll(void);
		void	wait(void) const;

		FT_DEFINE_EXCEPTION(StandardException, "ERROR: Standard Exception");
		FT_DEFINE_EXCEPTION(WaitException, "EXCEPTION CAUGHT IN PARENT: waiting");
};

std::string	parsePathInfo(const std::string& _uri, const std::string& scriptName);
std::string	parseQueryString(const std::string& _uri);
// std::string	getScriptName(const std::string& _uri, const std::string& name1, const std::string& name2);

#endif /* CGI_HPP */
