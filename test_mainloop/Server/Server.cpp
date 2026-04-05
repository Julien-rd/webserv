#include "Server.hpp"

Server::Server(const t_server_config& config):
_config(config) {}

Server::~Server(void) {}

// Server::Server(const Server& obj): // TODO Implement
// config(obj.config) {}

// Server&	Server::operator=(const Server& obj) {
// 	if (this == &obj) {
// 		return *this;
// 	}
// 	// TODO Implement
// 	return *this;
// }

void	Server::init(void) {
	_name = _config.name;
}

std::string	Server::getName(void) const {
	return _name;
}
