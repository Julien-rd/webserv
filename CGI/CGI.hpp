#pragma once
#include "../Client/HttpRequest/HttpRequest.hpp"
#include "../ConfigParser/Structs.hpp"
#include <string>

#include <sys/types.h>

#define KNOWN_EXTENSIONS_COUNT 2

// # define FT_THROW(class_name, exception_name)
// 	throw class_name##_##exception_name()

class Client;

typedef struct s_metaVariables {
  std::string auth_type;
  std::string content_length;
  std::string content_type;
  std::string gateway_interface;
  std::string path_info;
  std::string path_translated;
  std::string query_string;
  std::string remote_addr;
  std::string remote_host;
  std::string remote_ident;
  std::string remote_user;
  std::string request_method;
  std::string script_name;
  std::string server_name;
  std::string server_port;
  std::string server_protocol;
  std::string server_software;
  std::string script_filename;
} t_metaVariables;

class CGI {
    private:
        HttpRequest&                      _request;
        std::string                      _scriptName;
        t_metaVariables                  _meta;
        pid_t                            _pid;
        int                              _epfd;
        int                              _clientFd;
        const std::vector<t_cgi_config>& _cgiConfigs;
        const t_server&                  _serverConfig;

        std::string              _executable;
        std::vector<std::string> _argv;
        const char*              _envp[20];

        // static const std::string _knownExtensions[2];
        std::vector<std::string> knownExtensions;

        bool initPythonScript(void);
        void initMetaPython(void);
        bool setScriptAttributesPython(void);
        void setGETVariablesPython(void);
        void setPOSTVariablesPython(void);

        bool initPhpScript(void);
        void initMetaPhp(void);
        bool setScriptAttributesPhp(void);
        void setGETVariablesPhp(void);
        void setPOSTVariablesPhp(void);

        void execute(void);

    public:
        CGI(HttpRequest& request, int clientFd, int epfd,
            const t_server&                  serverConfig,
            const std::vector<t_cgi_config>& cgiConfigs);
        CGI(const CGI& obj);
        ~CGI(void);
        const CGI& operator=(const CGI& obj);

        // pid_t	getPid(void) const;
        // void	setPid(pid_t pid);

        int  pipefd[2];
        int  postPipefd[2];
        bool validateRequest(void) const;
        bool scriptFileExists(void) const;
        bool initCGI(void);
        bool pipeIO(void);
        bool redirectIO(void);
        bool spawnProcess(void);
        bool addPipeToEpoll(void);
        void wait(void) const;

        pid_t getPid(void) const;

        bool isCGIRequest(const HttpRequest& request);

        void reset();
        void reconstruct(const HttpRequest& request, int clientFd, int epfd,
            const t_server&                  serverConfig,
            const std::vector<t_cgi_config>& cgiConfigs);
        void setClientFd(const int fd);

};

std::string parsePathInfo(const std::string& _uri,
    const std::string& scriptName);
std::string parseQueryString(const std::string& _uri);
// std::string	getScriptName(const std::string& _uri, const std::string& name1,
// const std::string& name2);

