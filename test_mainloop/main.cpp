#include "Server/Server.hpp"

#include <exception>
#include <iostream>

int	main(void) {
	t_config		config = {1024, 4 * 1024 * 1024};
	Server			server(config);
	//TODO make fieldnames case INSENSITIVE
	try { // TODO Remove this try catch, because failing to initialize the server should exit, right? :D (basically exceptions should be used for things that could fail but prog still continues execution after). Also if u keep it, we shouldn't debug using exceptions prints
		server.initServer(); // TODO actually keep it, and change it to return error and exit :D
	}
	catch (std::exception& e) {
		std::cerr << e.what() << std::endl;
		return 1;
	}

	while (1) {
		// std::cout << "waiting for request \n";
		try {
			server.epollWait();
			server.loopReadyEvents();
		}
		catch (std::exception& e) {
			std::cerr << e.what() << std::endl;
			return 1;
		}
	}
}
