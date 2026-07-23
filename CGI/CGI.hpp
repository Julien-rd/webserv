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
  public:
    CGI();
    // CGI(HttpRequest                     *request,
    //     int                              clientFd,
    //     int                              epfd,
    //     const t_server                  &serverConfig,
    //     const std::vector<t_cgi_config> &cgiConfigs);
    // CGI(const CGI &obj);
    const CGI &operator=(const CGI &obj);
    ~CGI(void);

    bool validateRequest(void) const;
    bool scriptFileExists(void) const;
    bool initCGI(void);
    bool pipeIO(void);
    bool redirectIO(void);
    bool spawnProcess(void);
    bool addPipeToEpoll(void);
    void wait(void) const;
    bool isCGIRequest(const HttpRequest &request);
    void init(HttpRequest                     *request,
              int                              clientFd,
              int                              epfd,
              const t_server                  *serverConfig);
    void reset();
    void reconstruct(const HttpRequest               &request,
                     int                              clientFd,
                     int                              epfd,
                     const t_server                  &serverConfig,
                     const std::vector<t_cgi_config> &cgiConfigs);

    // getters
    pid_t getPid(void) const;

  private:
    const HttpRequest               *_request;
    std::string                      _scriptName;
    t_metaVariables                  _meta;
    pid_t                            _pid;
    int                              _epfd;
    int                              _clientFd;
    const std::vector<t_cgi_config> *_cgiConfigs;
    const t_server                  *_serverConfig;
    std::string                      _executable;
    std::vector<std::string>         _argv;
    const char                      *_envp[20];
    std::vector<std::string>         _knownExtensions;
    int                              _pipeFd[2];
    int                              _postPipeFd[2];

    // setters //Fix: these are private so they are not setters but initializers? rename accordingly
    bool setScriptAttributesPython(void);
    void setGETVariablesPython(void);
    void setPOSTVariablesPython(void);
    bool setScriptAttributesPhp(void);
    void setGETVariablesPhp(void);
    void setPOSTVariablesPhp(void);

    // inititalisers
    bool initPythonScript(void);
    void initMetaPython(void);
    bool initPhpScript(void);
    void initMetaPhp(void);

    void execute(void);
};

std::string parsePathInfo(const std::string &_uri, const std::string &scriptName);
std::string parseQueryString(const std::string &_uri);
// std::string	getScriptName(const std::string& _uri, const std::string& name1,
// const std::string& name2);
