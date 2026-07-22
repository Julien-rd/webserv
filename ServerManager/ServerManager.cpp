#include "ServerManager.hpp"

#include "../Utils/structs/ServerStructs.hpp"

#include <cerrno>
#include <csignal>
#include <cstddef>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <exception>
#include <fcntl.h>
#include <iostream>
#include <map>
#include <netinet/in.h>
#include <poll.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>
#include <vector>

ServerManager::ServerManager(const t_serverManagerContext &context)
        : _config(context.config)
        , _epfd(context.epfd)
        , _readyEventsCount(context.readyEventsCount)
        , _lastChecked(0)
        , _triggeredEvents(context.triggeredEvents) {}

ServerManager::~ServerManager(void) {
    for (std::map<int, Server>::iterator it = _servers.begin(); it != _servers.end(); it++) {
        it->second.closeClientFds();
        close(it->first);
    }
}

void ServerManager::addServerToMaps(int serverSocket, Server &server) {
    _servers.insert(std::pair<int, Server>(serverSocket, server));
    _serverToClientsMap.insert(std::pair<int, IntSet>(serverSocket, IntSet()));
}

void ServerManager::startServers(void) {
    int serverSocket;

    for (size_t i = 0; i < _config.servers.size(); ++i) {
        t_serverContext context = {
            _epfd, _config, i, _serverToClientsMap, _clients, _clientToServerMap};
        Server server(context);
        try {
            serverSocket = server.start();
        } catch (std::exception &e) {
            std::cerr << "WARNING: couldn't start server _" << server.getIdentifier()
                      << "_: " << e.what() << std::endl;
            continue;
        }
        addServerToMaps(serverSocket, server);
        std::cout << "Started Server __" << _servers.at(serverSocket).getIdentifier()
                  << "__ with socket " << serverSocket << " successfully" << std::endl;
    }
}

bool ServerManager::init(void) {
    startServers();
    if (_servers.size() == 0) {
        std::cout << "WARNING: no servers were started" << std::endl;
        return 1;
    }
    std::cout << std::endl;
    return 0;
}

void pp_memcpy(void *dst, void *src, size_t len) {
    for (size_t i = 0; i < len; i++) {
        static_cast<unsigned char *>(dst)[i] = static_cast<unsigned char *>(src)[i];
    }
}

/**
 * @brief Disconnects clients that have been idle longer than TIMEOUT (in s).
 *
 * To avoid checking every client on every event, timeout checks are performed
 * at most once every TIMEOUT seconds. Timed-out client file descriptors are
 * collected first, then disconnected afterward.
 */
void ServerManager::timeoutClients() {
    time_t now = time(NULL);
    if (_lastChecked + _config.clientTimeout < now) {
        _lastChecked = now;
        IntSet toErase;
        for (std::map<int, Client>::iterator it = _clients.begin(); it != _clients.end(); ++it) {
            Client &client = it->second;
            int     fd = client.getFd();
            if (fd != -1 && client.getLastActivity() + _config.clientTimeout < now)
                toErase.insert(fd);
        }
        for (IntSet::iterator it = toErase.begin(); it != toErase.end(); ++it)
            _servers.at(_clientToServerMap.at(*it)).closeConnection(*it);
    }
}

void ServerManager::loopReadyEvents(void) {
    timeoutClients();
    for (int i = 0; i < _readyEventsCount; ++i) {
        int fd = _triggeredEvents[i].data.fd;  // if it's Server or Client event, data union
        // will have Server or Client fd in fd. if its a
        // CGI event, data union will save two ints
        // (pipefd & clientFd) in u64 (or ptr)
        // std::cout << "fd in loop is: " << fd << std::endl;
        if (_servers.find(fd) != _servers.end()) {
            _servers.at(fd).handleServerEvent();
        } else if (_clientToServerMap.find(fd) != _clientToServerMap.end()) {
            _servers.at(_clientToServerMap[fd]).handleClientEvent(fd);
        } else {         /* is CGI's pipe fd */
            int fds[2];  // fds[0] is the pipefd. fds[1] is the owning client's fd.
            pp_memcpy(fds, &_triggeredEvents[i].data.u64, sizeof(uint64_t));
            _clients.at(fds[1]).readCGIPipe(fds[0]);
        }
    }
}
