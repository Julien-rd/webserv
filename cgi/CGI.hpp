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
	std::string	script_filename;
}	t_metaVariables;

class	CGI {
	private:
		std::string			scriptName;
		t_metaVariables		meta;
		pid_t				pid;
		int					pipefd[2];

		const char			*executable;
		char				*argv[3];
		const char			**envp;

		const std::string	pythonScriptName;
		const std::string	phpScriptName;

		void	initPythonScript(const HttpRequest& request);
		void	initMetaPython(const HttpRequest& request);
		void	setScriptAttributesPython(void);
		void	setGETVariablesPython(void);
		void	setPOSTVariablesPython(void);

		void	initPhpScript(const HttpRequest& request);
		void	initMetaPhp(const HttpRequest& request);
		void	setScriptAttributesPhp(void);
		void	setGETVariablesPhp(void);
		void	setPOSTVariablesPhp(void);

		void	execute(void);

	public:
		CGI(void);
		~CGI(void);

		// pid_t	getPid(void) const;
		// void	setPid(pid_t pid);

		void	validateRequest(const HttpRequest& request) const;
		void	initCGI(const HttpRequest& request);
		void	pipeIO(void);
		void	spawnProcess(void);
		void	wait(void) const;
		// void	redirectIO(void);

		FT_DEFINE_EXCEPTION(StandardException, "ERROR: Standard Exception");
		FT_DEFINE_EXCEPTION(WaitException, "EXCEPTION CAUGHT IN PARENT: waiting");
};

std::string	parsePathInfo(const std::string& _uri, const std::string& scriptName);
std::string	parseQueryString(const std::string& _uri);
std::string	getScriptName(const std::string& _uri, const std::string& name1, const std::string& name2);

#endif /* CGI_HPP */
