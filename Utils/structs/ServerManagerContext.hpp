#pragma once

#include "../../ConfigParser/Structs.hpp"

#include <sys/epoll.h>

typedef struct {
    const t_config           &config;
    int                       epfd;
    const int                &readyEventsCount;
    std::vector<epoll_event> &triggeredEvents;
} t_serverManagerContext;
