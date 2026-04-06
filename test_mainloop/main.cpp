#include "ServerManager/ServerManager.hpp"

#include <exception>
#include <iostream>

#define KB * 1024
#define MB * 1024 * 1024

t_config	parseConfig(const char *fileName) {
	(void)fileName;
	std::vector<t_server_config>	serverConfigs;
	std::string						global_1;
	std::string						global_2;

	t_server_config	server_conf = {global_1,
		global_2,
		8080,
		"intra not net",
		2 KB,
		4};
	t_server_config	server_conf_2 = {global_2,
		global_1,
		9090,
		"you not tube",
		4 KB,
		4};

	serverConfigs.push_back(server_conf);
	serverConfigs.push_back(server_conf_2);
	t_config	config = {global_1, global_2, serverConfigs};
	return config;
}

int	main(int argc, char **argv) {
	if (argc != 1) {
		return 1;
	}
	t_config		config = parseConfig(argv[1]);
	ServerManager	serverManager(config);
	//TODO make fieldnames case INSENSITIVE
	try { // TODO Remove this try catch, because failing to initialize the server should exit, right? :D (basically exceptions should be used for things that could fail but prog still continues execution after). Also if u keep it, we shouldn't debug using exceptions prints
		serverManager.init(); // TODO actually keep it, and change it to return error and exit :D
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
