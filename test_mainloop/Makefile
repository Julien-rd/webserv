CXX             = c++
CXXFLAGS        = -Wall -Wextra -Werror -std=c++98

SERVER_TARGET   = webserv
SERVER_OBJDIR   = webserv_obj
SERVER_SRCS     = server.cpp
SERVER_OBJS     = $(addprefix $(SERVER_OBJDIR)/, $(SERVER_SRCS:.cpp=.o))
SERVER_DEPS     = $(SERVER_OBJS:.o=.d)

CLIENT_TARGET   = client
CLIENT_OBJDIR   = client_obj
CLIENT_SRCS     = client.cpp
CLIENT_OBJS     = $(addprefix $(CLIENT_OBJDIR)/, $(CLIENT_SRCS:.cpp=.o))
CLIENT_DEPS     = $(CLIENT_OBJS:.o=.d)

all: $(SERVER_TARGET) $(CLIENT_TARGET)

server: $(SERVER_TARGET)
client: $(CLIENT_TARGET)

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

clean:
	rm -rf $(SERVER_OBJDIR) $(CLIENT_OBJDIR)

fclean: clean
	rm -f $(SERVER_TARGET) $(CLIENT_TARGET)

re: fclean all

-include $(SERVER_DEPS)
-include $(CLIENT_DEPS)

.PHONY: all clean fclean re server client