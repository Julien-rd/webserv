#ifndef CGI_HPP
# define CGI_HPP

# include <string>
# include <exception>

class	CGI {
	private:
		char	*content;
		pid_t	pid;
		int		pipefd[2];
		char	*path;
		char	*argv[2];
		char	**envp;

	public:
		CGI(char *content);
		~CGI(void);

		pid_t	getPid(void) const;
		void	setPid(pid_t pid);

		void	validateContent(void) const;
		void	parseContent(char **newEnvp);
		void	spawnProcess(void);
		void	pipeIO(void);
		void	redirectIO(void);
		void	execute(void);

		class CGIStandardException: public std::exception {
			public: const char *what(void) const throw() {return "ERROR: CGI Exception";}
		};
		class CGIWaitException: public std::exception {
			public: const char *what(void) const throw() {return "RUNNING: Parent is waiting for cgi process";}
		};
};

#endif /* CGI_HPP */