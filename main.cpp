#include "ConfigParser/Parser.hpp"
#include "ConfigParser/Structs.hpp"
#include "Logger/Logger.hpp"
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
    std::cout << "Exiting with signal: " << sig << std::endl;  // fix how to log here? should we?
    // _exit(sig);
    throw std::exception();  // TODO we shouldn't use exceptions for normal
                             // logic
                             // routes, except that ctrl+c is not normal??? idk
}

static bool parseLogLevelArg(const std::string &arg, Level::Value &outLevel) {
    const std::string prefix = "--log-level=";
    if (arg.compare(0, prefix.size(), prefix) != 0)
        return false;

    std::string value = arg.substr(prefix.size());

    if (value == "debug")
        outLevel = Level::DEBUG;
    else if (value == "info")
        outLevel = Level::INFO;
    else if (value == "warning")
        outLevel = Level::WARNING;
    else if (value == "error")
        outLevel = Level::ERROR;
    else
        return false;

    return true;
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

    if (argc > 3 || argc < 2) {
        log(Level::ERROR, "Usage: ./webserv [config_file] [--log-level=debug|info|warning|error]");
        return 1;
    }

    if (!validConfigFile(argv[1])) {
        log(Level::ERROR, "invalid config file.\nUsage: ./webserv FILENAME.pps");
        return 1;
    }

    if (parseConfigFile(config, argv[1])) {
        log(Level::ERROR, "parsing configuration file failed.");
        return 1;
    }

    Logger::getInstance().setLevel(config.logLvl);

    if (argc == 3) {
        Level::Value argLevel;
        if (!parseLogLevelArg(argv[2], argLevel)) {
            log(Level::ERROR, "invalid --log-level value. Use debug|info|warning|error");
            return 1;
        }
        Logger::getInstance().setLevel(argLevel);
    }

    Poller poller;
    if (poller.createEpoll() != 0) {
        return 1;
    }

    t_serverManagerContext context = {
        config, poller.getEpfd(), poller.getReadyEventsCountRef(), poller.getTriggeredEventsRef()};
    ServerManager serverManager(context);

    if (serverManager.init())
        return 1;

    while (1) {
        try {
            poller.epollWait();
            serverManager.loopReadyEvents();
        } catch (std::exception &e) {
            log(Level::ERROR, e.what());
            return 1;
        }
    }
}
