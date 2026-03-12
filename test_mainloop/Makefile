CXX			= c++
CXXFLAGS	= -Wall -Wextra -Werror -std=c++98 #-fsanitize=address

SERVER_TARGET	= webserv
SERVER_OBJDIR	= webserv_obj
SERVER_DEPDIR	= $(SERVER_OBJDIR)
SERVER_SRCS		= server.cpp

CLIENT_TARGET	= client
CLIENT_OBJDIR	= client_obj
CLIENT_DEPDIR	= $(CLIENT_OBJDIR)
CLIENT_SRCS		= client.cpp


SERVER_OBJS		= $(addprefix $(SERVER_OBJDIR)/, $(SERVER_SRCS:.cpp=.o))
SERVER_DEPS		= $(addprefix $(SERVER_DEPDIR)/, $(SERVER_SRCS:.cpp=.d))

CLIENT_OBJS		= $(addprefix $(CLIENT_OBJDIR)/, $(CLIENT_SRCS:.cpp=.o))
CLIENT_DEPS		= $(addprefix $(CLIENT_DEPDIR)/, $(CLIENT_SRCS:.cpp=.d))

all: $(SERVER_TARGET) $(CLIENT_TARGET)

server: $(SERVER_OBJS)
	$(CXX) $(CXXFLAGS) $(SERVER_OBJS) -o $(SERVER_TARGET)

client: $(CLIENT_OBJS)
	$(CXX) $(CXXFLAGS) $(CLIENT_OBJS) -o $(CLIENT_TARGET)

$(SERVER_TARGET): $(SERVER_OBJS)
	$(CXX) $(CXXFLAGS) $(SERVER_OBJS) -o $(SERVER_TARGET)

$(SERVER_OBJDIR)/%.o: %.cpp | $(SERVER_OBJDIR)
	$(CXX) $(CXXFLAGS) -MMD -MP -c $< -o $@

$(SERVER_OBJDIR):
	mkdir -p $(SERVER_OBJDIR)

$(CLIENT_TARGET): $(CLIENT_OBJS)
	$(CXX) $(CXXFLAGS) $(CLIENT_OBJS) -o $(CLIENT_TARGET)


$(CLIENT_OBJDIR)/%.o: %.cpp | $(CLIENT_OBJDIR)
	$(CXX) $(CXXFLAGS) -MMD -MP -c $< -o $@

$(CLIENT_OBJDIR):
	mkdir -p $(CLIENT_OBJDIR)

-include $(SERVER_DEPS)

-include $(CLIENT_DEPS)

clean:
	rm -rf $(SERVER_OBJDIR) $(CLIENT_OBJDIR)

fclean: clean
	rm -f $(SERVER_TARGET) $(CLIENT_TARGET)

re: fclean all

.PHONY: all clean fclean re