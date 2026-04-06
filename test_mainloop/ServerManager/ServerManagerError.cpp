#include "ServerManager.hpp"

#include "../headers/structs/ErrorType.hpp"

#include <iostream>

#include <cstring>
#include <cerrno>

static void	custom_perror(const char *spec, char *errnoMsg) {
  std::cout << spec << errnoMsg << "\n";
}

int	ServerManager::error_msg(ErrorType type) {
  int ret = 1;
  switch (type) {
  case ERR_EPOLL_WAIT:
    custom_perror("epoll_wait: ", strerror(errno));
    break;
  case ERR_SOCKET:
    custom_perror("socket: ", strerror(errno));
    break;
  case ERR_ACCEPT:
    custom_perror("accept: ", strerror(errno));
    break;
  case ERR_FCNTL:
    custom_perror("fcntl: ", strerror(errno));
    break;
  case ERR_SETSOCKOPT:
    custom_perror("setsockopt: ", strerror(errno));
    break;
  case ERR_EPOLL_CREATE1:
    custom_perror("epoll_create1: ", strerror(errno));
    break;
  case ERR_EPOLL_CTL:
    custom_perror("epoll_ctl: ", strerror(errno));
    break;
  case ERR_BIND:
    custom_perror("bind: ", strerror(errno));
    break;
  case ERR_LISTEN:
    custom_perror("listen: ", strerror(errno));
    break;
  case ERR_RECV:
    custom_perror("recv: ", strerror(errno));
    break;
  case ERR_CLOSE:
    custom_perror("close: ", strerror(errno));
    break;
  }
  return ret;
}

