#ifndef CGI_HPP
#define CGI_HPP

#include "../Client/HttpRequest/HttpRequest.hpp"
#include "../ConfigParser/Structs.hpp"
#include <exception>
#include <string>

#include <sys/types.h>

#define FT_DEFINE_EXCEPTION(exception_name, message)                           \
  class exception_name : public std::exception {                               \
  public:                                                                      \
    const char* what(void) const throw() { return message; }                   \
  }

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
  HttpRequest                      request;
  std::string                      scriptName;
  t_metaVariables                  meta;
  pid_t                            pid;
  int                              epfd;
  int                              clientFd;
  const std::vector<t_cgi_config>& cgiConfigs;
  const t_server&                  serverConfig;

  std::string              executable;
  std::vector<std::string> argv;
  const char*              envp[20];

  // static const std::string _knownExtensions[2];
  std::vector<std::string> knownExtensions;

  void initPythonScript(void);
  void initMetaPython(void);
  void setScriptAttributesPython(void);
  void setGETVariablesPython(void);
  void setPOSTVariablesPython(void);

  void initPhpScript(void);
  void initMetaPhp(void);
  void setScriptAttributesPhp(void);
  void setGETVariablesPhp(void);
  void setPOSTVariablesPhp(void);

  void execute(void);

public:
  CGI(const HttpRequest& request, int clientFd, int epfd,
      const t_server&                  serverConfig,
      const std::vector<t_cgi_config>& cgiConfigs);
  CGI(const CGI& obj);
  ~CGI(void);
  const CGI& operator=(const CGI& obj);

  // pid_t	getPid(void) const;
  // void	setPid(pid_t pid);

  int  pipefd[2];
  bool validateRequest(void) const;
  bool scriptFileExists(void) const;
  void initCGI(void);
  void pipeIO(void);
  void redirectIO(void);
  void spawnProcess(void);
  void addPipeToEpoll(void);
  void wait(void) const;

  pid_t getPid(void) const;

  bool isCGIRequest(const HttpRequest& request);

  void reset();
  void reconstruct(const HttpRequest& request, int clientFd, int epfd,
                   const t_server&                  serverConfig,
                   const std::vector<t_cgi_config>& cgiConfigs);
  void setClientFd(const int fd);

  FT_DEFINE_EXCEPTION(StandardException, "ERROR: Standard Exception");
  FT_DEFINE_EXCEPTION(WaitException, "EXCEPTION CAUGHT IN PARENT: waiting");
};

std::string parsePathInfo(const std::string& _uri,
                          const std::string& scriptName);
std::string parseQueryString(const std::string& _uri);
// std::string	getScriptName(const std::string& _uri, const std::string& name1,
// const std::string& name2);

#endif /* CGI_HPP */
