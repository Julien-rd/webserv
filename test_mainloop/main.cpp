#include "ConfigParser/Parser.hpp"
#include "ConfigParser/Structs.hpp"
#include "Poller/Poller.hpp"
#include "ServerManager/ServerManager.hpp"
#include "headers/structs/ServerManagerContext.hpp"

#include <csetjmp>
#include <cstdlib>
#include <exception>
#include <iostream>

#include <csignal>

/* TODO Claude says: In a single-process, non-blocking architecture, the rule is
simple: Any fd you need to wait on must go through epoll. Waiting on it any
other way blocks the loop. This applies to sockets, pipes, timers (timerfd),
signals (signalfd) — anything. CGI pipes are no exception. */
void signalHandler(int sig) {
  std::cout << "Exiting with signal: " << sig << std::endl;
  // _exit(sig);
  throw std::exception(); // TODO we shouldn't use exceptions for normal logic
                          // routes, except that ctrl+c is not normal??? idk
}

int main(int argc, char** argv) {
  signal(SIGINT, signalHandler);
  t_config config;
  if (argc != 2) {
    std::cerr << "ERROR: provide exactly one argument ./webserv [filename] "
              << std::endl;
    return 1;
  }
  if (parseConfigFile(config, argv[1])) {
    std::cerr << "ERROR: parsing configuration file failed. " << std::endl;
    return 1;
  }
  Poller poller;
  if (poller.createEpoll() != 0) {
    return 1;
  }
  t_serverManagerContext context = {config, poller.getEpfd(),
                                    poller.getReadyEventsCountRef(),
                                    poller.getTriggeredEventsRef()};
  ServerManager          serverManager(context);
  // TODO make fieldnames case INSENSITIVE
  if (serverManager.init()) {
    return 1;
  }
  while (1) {
    try {
      poller.epollWait();
      serverManager.loopReadyEvents();
    } catch (std::exception& e) {
      // std::cerr << e.what() << std::endl;
      return 1;
    }
  }
}
