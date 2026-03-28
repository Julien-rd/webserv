#include "Server.hpp"
#include <iostream>
#include <cstring>

void	custom_perror(const char *spec, char *errnoMsg) {
  std::cout << spec << errnoMsg << "\n";
}

int	Server::error_msg(Type type) {
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
  case ERR_READ:
    if (errno == EAGAIN || errno == EWOULDBLOCK)
      ret = 0;
    else
      custom_perror("read: ", strerror(errno));
    break;
  }
//   if (ret == 1)
//     cleanup(this->openFds); TODO do this
  return ret;
}

