#pragma once
enum ErrorType {
    ERR_EPOLL_WAIT,
    ERR_SOCKET,
    ERR_ACCEPT,
    ERR_FCNTL,
    ERR_SETSOCKOPT,
    ERR_EPOLL_CREATE,
    ERR_EPOLL_CTL,
    ERR_BIND,
    ERR_LISTEN,
    ERR_RECV,
    ERR_CLOSE
};

class Error {
  public:
    void error_msg(ErrorType type);
};
