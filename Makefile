NAME = webserv

CXX = c++

CXXFLAGS = -Wall -Wextra -Werror -MMD -MP -std=c++98 -pedantic -g #-fsanitize=address,undefined,bounds,float-divide-by-zero

SOURCES = main.cpp
CLASS_SOURCES = Error/Error.cpp \
				Poller/Poller.cpp \
				ServerManager/ServerManager.cpp \
				Server/Server.cpp \
				CGI/CGI.cpp CGI/CGI_helpers.cpp CGI/CGI_php.cpp CGI/CGI_python.cpp CGI/CGIResponse.cpp \
				Client/Client.cpp \
				Client/HttpRequest/HttpRequest.cpp Client/HttpRequest/HttpRequestGetters.cpp \
				Client/HttpRequest/HttpRequestHelpers.cpp \
				Client/HttpResponse/HttpResponse.cpp Client/HttpResponse/ReasonPhrase.cpp \
				ConfigParser/DirectiveHandlers.cpp ConfigParser/EvalTree.cpp ConfigParser/Parser.cpp \
				ConfigParser/ParseTree.cpp ConfigParser/Tokenizer.cpp

ALL_SOURCES = $(SOURCES) $(CLASS_SOURCES)

HEADERS = ServerManager.hpp HttpRequest.hpp # TODO add the rest
TEMPLATE = 

OBJ_DIR = objects/
OBJS = $(ALL_SOURCES:%.cpp=$(OBJ_DIR)%.o)
DEPS = $(OBJS:.o=.d)

all: $(NAME)

$(NAME): $(OBJS)
	@$(CXX) $(CXXFLAGS) $(OBJS) -o $@

$(OBJ_DIR)%.o: %.cpp
	@mkdir -p $(dir $@)
	@$(CXX) $(CXXFLAGS) -c $< -o $@

run: all
	./$(NAME) ConfigFiles/config.pps

runval: CXXFLAGS:= -Wall -Wextra -Werror -std=c++98 -pedantic -g
runval: re
	valgrind --leak-check=full --show-leak-kinds=all --track-fds=all ./$(NAME) ConfigFiles/config.pps
	
runsan: CXXFLAGS:= -std=c++98 -g -fsanitize=address,leak,undefined,bounds,float-divide-by-zero
runsan: re
	./$(NAME) ConfigFiles/config.pps

clean:
	@rm -rf $(OBJ_DIR)

fclean: clean
	@rm -rf $(NAME)

re: fclean all

-include $(DEPS)


# CXX             = c++
# CXXFLAGS        = -Wall -Wextra -Werror -std=c++98

# SERVER_TARGET   = webserv
# SERVER_OBJDIR   = webserv_obj
# SERVER_SRCS     = Server.cpp ServerError.cpp Client/HttpRequest/HttpRequest.cpp Client/HttpRequest/HttpRequest.hpp Client/HttpRequest/HttpRequestGetters.cpp Client/HttpRequest/HttpRequestHelpers.cpp
# SERVER_OBJS     = $(addprefix $(SERVER_OBJDIR)/, $(SERVER_SRCS:.cpp=.o))
# SERVER_DEPS     = $(SERVER_OBJS:.o=.d)

# CLIENT_TARGET   = client
# CLIENT_OBJDIR   = client_obj
# CLIENT_SRCS     = client.cpp
# CLIENT_OBJS     = $(addprefix $(CLIENT_OBJDIR)/, $(CLIENT_SRCS:.cpp=.o))
# CLIENT_DEPS     = $(CLIENT_OBJS:.o=.d)

# all: $(SERVER_TARGET) $(CLIENT_TARGET)

# $(SERVER_TARGET): $(SERVER_OBJS)
# 	$(CXX) $(CXXFLAGS) $(SERVER_OBJS) -o $(SERVER_TARGET)

# $(SERVER_OBJDIR)/%.o: %.cpp | $(SERVER_OBJDIR)
# 	$(CXX) $(CXXFLAGS) -MMD -MP -c $< -o $@

# $(SERVER_OBJDIR):
# 	mkdir -p $(SERVER_OBJDIR)

# $(CLIENT_TARGET): $(CLIENT_OBJS)
# 	$(CXX) $(CXXFLAGS) $(CLIENT_OBJS) -o $(CLIENT_TARGET)

# $(CLIENT_OBJDIR)/%.o: %.cpp | $(CLIENT_OBJDIR)
# 	$(CXX) $(CXXFLAGS) -MMD -MP -c $< -o $@

# $(CLIENT_OBJDIR):
# 	mkdir -p $(CLIENT_OBJDIR)

# clean:
# 	rm -rf $(SERVER_OBJDIR) $(CLIENT_OBJDIR)

# fclean: clean
# 	rm -f $(SERVER_TARGET) $(CLIENT_TARGET)

# re: fclean all

# -include $(SERVER_DEPS)
# -include $(CLIENT_DEPS)

# .PHONY: all clean fclean re server client