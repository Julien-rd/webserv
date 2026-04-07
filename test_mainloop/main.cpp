#include "Client/HttpRequest/HttpRequest.hpp"
#include "ServerManager/ServerManager.hpp"

#include <csetjmp>
#include <exception>
#include <iostream>

#include <csignal>

void	signalHandler(int sig) {
	std::cout << "Exiting " << sig << std::endl;
	throw std::exception(); // TODO we shouldn't use exceptions for normal logic routes, except that ctrl+c is not normal??? idk
}

t_config	parseConfig(const char *fileName) {
	(void)fileName;
	std::vector<t_server_config>	serverConfigs;

	t_server_config	server_conf = {
		1024,
		8080,
		"intra not net",
		4};
	t_server_config	server_conf_2 = {
		1024,
		9090,
		"you not tube",
		4};

	serverConfigs.push_back(server_conf);
	serverConfigs.push_back(server_conf_2);
	t_config	config = {1024, serverConfigs};
	return config;
}

int	main(int argc, char **argv) {
	(void)argc, (void)argv;
	signal(SIGINT, signalHandler);
	t_config		config = parseConfig(argv[1]);
	ServerManager	serverManager(config);
	//TODO make fieldnames case INSENSITIVE
	try {
		serverManager.init();
	}
	catch (std::exception& e) {
		std::cerr << e.what() << std::endl;
		return 1;
	}

	while (1) {
		// std::cout << "waiting for request \n";
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
