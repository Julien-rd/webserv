#pragma once
#include "../Error/Error.hpp"

#include <sys/epoll.h>
#include <vector>
#include <cstring>
#include <cerrno>

class Poller : public Error {
  private:
    int                      _epfd;
    int                      _readyEventsCount;
    std::vector<epoll_event> _triggeredEvents;

  public:
    Poller(void);
    ~Poller(void);

    bool createEpoll(void);
    int  epollWait(void);

    std::vector<epoll_event> &getTriggeredEventsRef(void);
    int                      &getReadyEventsCountRef(void);
    int                       getEpfd(void) const;
};
