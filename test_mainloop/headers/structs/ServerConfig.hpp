#ifndef SERVER_CONFIG_STRUCT_HPP
# define SERVER_CONFIG_STRUCT_HPP

# include <string>

typedef struct s_server_config {
	size_t		maxClients;
	int			host;
	std::string	name;
	size_t		maxBodySize;
}	t_server_config;


#endif /* SERVER_CONFIG_STRUCT_HPP */
