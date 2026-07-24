#include "ConfigParser/Parser.hpp"
#include "ConfigParser/Structs.hpp"
#include "Poller/Poller.hpp"
#include "ServerManager/ServerManager.hpp"
#include "Utils/debugPrint.cpp"
#include "Utils/structs/ServerManagerContext.hpp"

#include <csetjmp>
#include <csignal>
#include <cstdlib>
#include <exception>
#include <iostream>

/* TODO Claude says: In a single-process, non-blocking architecture, the rule is
simple: Any fd you need to wait on must go through epoll. Waiting on it any
other way blocks the loop. This applies to sockets, pipes, timers (timerfd),
signals (signalfd) — anything. CGI pipes are no exception. */
void signalHandler(int sig) {
    std::cout << "Exiting with signal: " << sig << std::endl;
    // _exit(sig);
    throw std::exception();  // TODO we shouldn't use exceptions for normal
                             // logic
                             // routes, except that ctrl+c is not normal??? idk
}

size_t ft_strlen(char *str) {
    size_t ret = 0;
    while (str[ret]) {
        ret++;
    }
    return ret;
}

bool validConfigFile(char *fileName) {
    size_t fileLen = ft_strlen(fileName);
    if (fileLen < 5) {
        return false;
    }
    if (fileName[fileLen - 5] == '/') {
        return false;
    }
    if (std::string(&fileName[fileLen - 4]) != ".pps") {
        return false;
    }
    int fd = open(fileName, O_RDONLY);
    if (fd == -1) {
        return false;
    }
    close(fd);
    return true;
}

int main(int argc, char **argv) {
    signal(SIGINT, signalHandler);
    signal(SIGPIPE, SIG_IGN);
    t_config config;
    if (argc != 2) {
        std::cerr << "ERROR: provide exactly one argument ./webserv FILENAME.pps " << std::endl;
        return 1;
    }
    if (!validConfigFile(argv[1])) {
        std::cerr << "ERROR: invalid config file." << std::endl
                  << "  usage: ./webserv FILENAME.pps" << std::endl;
        return 1;
    }
    if (parseConfigFile(config, argv[1])) {
        std::cerr << "ERROR: parsing configuration file failed. " << std::endl;
        return 1;
    }
    // printConfig(config);
    Poller poller;
    if (poller.createEpoll() != 0) {
        return 1;
    }
    t_serverManagerContext context = {
        config, poller.getEpfd(), poller.getReadyEventsCountRef(), poller.getTriggeredEventsRef()};
    ServerManager serverManager(context);
    // TODO make fieldnames case INSENSITIVE
    if (serverManager.init()) {
        return 1;
    }
    while (1) {
        try {
            poller.epollWait();
            serverManager.loopReadyEvents();
        } catch (std::exception &e) {
            std::cerr << e.what() << std::endl;
            return 1;
        }
    }
}
