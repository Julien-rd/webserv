#include "Poller.hpp"

#include <unistd.h>

#include "../Logger/Logger.hpp"

#define MAX_EVENTS 2048

Poller::Poller(void) : _epfd(-1) { _triggeredEvents.reserve(MAX_EVENTS); }

Poller::~Poller(void) {
    if (_epfd != -1) {
        close(_epfd);
    }
}

bool Poller::createEpoll(void) {
    _epfd = epoll_create(0);
    if (_epfd == -1) {
        log(Level::ERROR, "epoll_create: " + std::string(strerror(errno)));
        return 1;
    }
    return 0;
}

int Poller::epollWait(void) {
    _readyEventsCount = epoll_wait(_epfd,
                                   _triggeredEvents.data(),
                                   MAX_EVENTS,
                                   1000);  // NEXT TODO: 1000ms timeout and check for client last
                                           // activity over 10 difference to time()
    if (_readyEventsCount == -1) {
        log(Level::ERROR, "epoll_wait: " + std::string(strerror(errno)));
        // perror(strerror(errno));
        return -1;
    }
    return _readyEventsCount;
}

std::vector<epoll_event> &Poller::getTriggeredEventsRef(void) { return _triggeredEvents; }

int &Poller::getReadyEventsCountRef(void) { return _readyEventsCount; }

int Poller::getEpfd() const { return _epfd; }
