#ifndef SERVER_CONFIG_STRUCT_HPP
# define SERVER_CONFIG_STRUCT_HPP

# include <string>

typedef struct s_server_config {
	std::string	globalDirective_1;
	std::string	globalDirective_2;
	std::string	host;
	std::string	name;
	size_t		maxBodySize;
	size_t		maxClients;
}	t_server_config;


#endif /* SERVER_CONFIG_STRUCT_HPP */
