CXX		    = c++
CXXFLAGS	= -Wall -Wextra -Werror -std=c++98 #-fsanitize=address

NAME		= webserv

OBJDIR		= obj
DEPDIR		= $(OBJDIR)

SRCS		= HttpRequest.cpp HttpRequestHelpers.cpp main.cpp 

OBJS		= $(addprefix $(OBJDIR)/, $(SRCS:.cpp=.o))
DEPS		= $(addprefix $(DEPDIR)/, $(SRCS:.cpp=.d))

all: $(NAME)

$(NAME): $(OBJS)
	$(CXX) $(CXXFLAGS) $(OBJS) -o $(NAME)

$(OBJDIR)/%.o: %.cpp | $(OBJDIR)
	$(CXX) $(CXXFLAGS) -MMD -MP -c $< -o $@

$(OBJDIR):
	mkdir -p $(OBJDIR)

-include $(DEPS)

clean:
	rm -rf $(OBJDIR)

fclean: clean
	rm -f $(NAME)

re: fclean all

.PHONY: all clean fclean re