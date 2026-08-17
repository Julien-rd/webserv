NAME = webserv

CXX = c++

CXXFLAGS = -Wall -Wextra -Werror -MMD -MP -std=c++98 -pedantic -g #-fsanitize=address,undefined,bounds,float-divide-by-zero

SOURCES = main.cpp
CLASS_SOURCES = Error/Error.cpp \
				Poller/Poller.cpp \
				Logger/Logger.cpp \
				ServerManager/ServerManager.cpp \
				Server/Server.cpp \
				CGI/CGI.cpp CGI/CGI_helpers.cpp CGI/CGIResponse.cpp \
				Client/Client.cpp \
				Client/HttpRequest/HttpRequest.cpp Client/HttpRequest/HttpRequestGetters.cpp \
				Client/HttpRequest/HttpRequestHelpers.cpp \
				Client/HttpResponse/HttpResponse.cpp Client/HttpResponse/ReasonPhrase.cpp \
				ConfigParser/DirectiveHandlers.cpp ConfigParser/EvalTree.cpp ConfigParser/Parser.cpp \
				ConfigParser/ParseTree.cpp ConfigParser/Tokenizer.cpp

ALL_SOURCES = $(SOURCES) $(CLASS_SOURCES)

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
	./$(NAME) Pages/PasswordManager/config.pps

runval: CXXFLAGS:= -Wall -Wextra -Werror -std=c++98 -pedantic -g
runval: re
	valgrind --leak-check=full --show-leak-kinds=all --track-fds=all ./$(NAME) Pages/PasswordManager/config.pps
	
runsan: CXXFLAGS:= -std=c++98 -g -fsanitize=address,leak,undefined,bounds,float-divide-by-zero
runsan: re
	./$(NAME) Pages/PasswordManager/config.pps

clean:
	@rm -rf $(OBJ_DIR)

fclean: clean
	@rm -rf $(NAME)
	
-include $(DEPS)

re: fclean all

.PHONY: all clean fclean re

