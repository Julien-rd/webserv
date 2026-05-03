#ifndef POLLER_CLASS_HPP
#define POLLER_CLASS_HPP

#include "../Error/Error.hpp"

#include <vector>

#include <sys/epoll.h>

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

  std::vector<epoll_event>& getTriggeredEventsRef(void);
  int&                      getReadyEventsCountRef(void);
  int                       getEpfd(void) const;
};

#endif /* POLLER_CLASS_HPP */
