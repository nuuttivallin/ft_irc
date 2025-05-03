CFILES = main.cpp Client.cpp Server.cpp Channel.cpp \

OFILES = $(CFILES:.cpp=.o)

C++ = c++

CFLAGS = -Wall -Wextra -Werror -std=c++98

NAME = ircserv

all: $(NAME)

$(NAME): $(OFILES)
	$(C++) $(CFLAGS) -o $(NAME) $(OFILES)

%.o:%.cpp
	$(C++) $(CFLAGS) -o $@ -c $^

clean:
	rm -f $(OFILES)

fclean: clean
	rm -f $(NAME)

re: fclean all

.PHONY: all clean fclean re
