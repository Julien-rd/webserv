#pragma once
#include "../CGI/CGIResponse.hpp"
#include "../Client/HttpRequest/HttpRequest.hpp"
#include "../ConfigParser/Structs.hpp"

#include <string>
#include <sys/types.h>

#define KNOWN_EXTENSIONS_COUNT 2

class Client;

enum errPosition { EPOLL, BEFORE_EPOLL };

enum status { RESPONSE_READY, RESPONSE_PENDING, RESPONSE_ERR };

enum envStates { PYTHON, PHP, METHOD_GET, METHOD_POST };

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
    std::string http_cookie;
    std::string server_software;
    std::string script_filename;
} t_metaVariables;

class CGI {
  public:
    CGI();
    const CGI &operator=(const CGI &obj);
    ~CGI(void);

    bool handleCGI(void);
    int  buildResponse(int pipeReadFd);
    bool isCGIRequest(const HttpRequest &request);
    void init(HttpRequest *request, int clientFd, int epfd, int sid, const t_config *config);
    void reset();
    void flushWriteBuffer(void);

    // getters
    unsigned int       getIdentifier() const;
    pid_t              getPid(void) const;
    // int                getPipeFd(void) const;
    int                getReadFd(void) const;
    int                getPostFd(void) const;
    const CGIResponse &getResponse();
    void               setReadFd(int fd);
    void               setPostFd(int fd);
    void                setKnownExtensions(void);

  private:
    std::string  _CGIResponseStream;
    std::string  _CGIResponseStr;
    ssize_t      _CGIResponseLen;
    pid_t        _CGIPid;
    CGIResponse  _CGIResponse;
    unsigned int _CGIIdentifier;

    HttpRequest                     *_request;
    std::string                      _scriptName;
    t_metaVariables                  _meta;
    pid_t                            _pid;
    int                              _sid;
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
    size_t                           _writeOffset;
    std::vector<char>                _writeTotal;
    time_t                           _lastProgressTime;
    int                              _postRegisteredFd;
    int                              _readRegisteredFd;

    // setters //Fix: these are private so they are not setters but initializers? rename accordingly
    void setEnv(int type, int state);

    // inititalisers
    bool setScriptAttributes(int type);
    bool initScript(int type);
    void setMeta(std::string &field, const std::string &key, const std::string &value);
    void initMeta(int type);

    void execute(void);
    bool doCGI(void);
    bool validateRequest(void) const;
    bool scriptFileExists(void) const;
    bool initCGI(void);
    bool pipeIO(void);
    bool redirectIO(void);
    bool spawnProcess(void);
    bool addPipeToEpoll(void);
    void errHandler(int fd, errPosition pos);
};

std::string parsePathInfo(const std::string &_uri, const std::string &scriptName);
