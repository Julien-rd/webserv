#include "ParseConfig/Structs.hpp"
#include "ServerManager/ServerManager.hpp"
#include "ParseConfig/Parser.hpp"
#include <csetjmp>
#include <cstdlib>
#include <exception>
#include <iostream>

#include <csignal>
#include <stdexcept>

void	signalHandler(int sig) {
	std::cout << "Exiting with signal: " << sig << std::endl;
	// _exit(sig);
	throw std::exception(); // TODO we shouldn't use exceptions for normal logic routes, except that ctrl+c is not normal??? idk
}

int	main(int argc, char **argv) {
	(void)argc, (void)argv;
	signal(SIGINT, signalHandler);
	t_config  config{};
	try {
    	if (argc != 2)
            throw std::runtime_error("Error\nprovide exactly one argument ./webserv [filename]");
	    parser(config, argv[1]);
	} catch (std::exception &e) {
        return -1;
	}
	ServerManager	serverManager(config);
	//TODO make fieldnames case INSENSITIVE
	if (serverManager.init()) {
	std::cerr << "Couldn't start." << std::endl;
	return 1;
	}
	while (1) {
	try {
	serverManager.epollWait();
	serverManager.loopReadyEvents();
	}
	catch (std::exception& e) {
	std::cerr << e.what() << std::endl;
	return 1;
	}
	}
}
