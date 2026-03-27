NAME = cgi

CXX = c++

CXXFLAGS = -Wall -Wextra -Werror -MMD -MP -std=c++98 -pedantic -g #-fsanitize=address,undefined,bounds,float-divide-by-zero

SOURCES = CGI_helpers.cpp
CLASS_SOURCES = CGI.cpp CGI_python.cpp CGI_php.cpp HttpRequest.cpp HttpRequestHelpers.cpp
ALL_SOURCES = $(SOURCES) $(CLASS_SOURCES)

HEADERS = CGI.hpp HttpRequest.hpp
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

run: re
	./$(NAME)

runval: CXXFLAGS:= -Wall -Wextra -Werror -std=c++98 -pedantic -g
runval: re
	valgrind --leak-check=full --show-leak-kinds=all ./$(NAME) input.txt
	
runsan: CXXFLAGS:= -std=c++98 -g -fsanitize=address,leak,undefined,bounds,float-divide-by-zero
runsan: re
	./$(NAME)

clean:
	@rm -rf $(OBJ_DIR)

fclean: clean
	@rm -rf $(NAME)

re: fclean all

-include $(DEPS)
