#pragma once

#include "../../ConfigParser/Structs.hpp"
#include "../../Poller/Poller.hpp"

typedef struct s_serverManagerContext {
  const t_config&           config;
  int                       epfd;
  const int&                readyEventsCount;
  std::vector<epoll_event>& triggeredEvents;
} t_serverManagerContext;
