# **************************************************************************** #
#                                                                              #
#                                                         :::      ::::::::    #
#    Makefile                                           :+:      :+:    :+:    #
#                                                     +:+ +:+         +:+      #
#    By: dromansk <marvin@42.fr>                    +#+  +:+       +#+         #
#                                                 +#+#+#+#+#+   +#+            #
#    Created: 2019/02/28 21:18:25 by dromansk          #+#    #+#              #
#    Updated: 2019/11/13 00:38:19 by dromansk         ###   ########.fr        #
#                                                                              #
# **************************************************************************** #

NAME = gbmu

SRCS = ./src/*.cc

COMP = g++ -w -O3

L = -lSDL2

all: $(NAME)

$(NAME):
	$(COMP) $(SRCS) -o $(NAME) $(O) $(L)

clean:
	rm -rf $(NAME)

fclean: clean
	rm -rf $(NAME)

re: fclean all

lre: fclean
	$(COMP) -fsanitize=address $(SRCS) -o $(NAME) $(O) $(L)

vre: fclean
	$(COMP) -g -pg $(SRCS) -o $(NAME) $(O) $(L)

test: re
	./$(NAME) ./roms/Tetris.gb > log.txt
