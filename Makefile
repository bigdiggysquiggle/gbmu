# **************************************************************************** #
#                                                                              #
#                                                         :::      ::::::::    #
#    Makefile                                           :+:      :+:    :+:    #
#                                                     +:+ +:+         +:+      #
#    By: dromansk <marvin@42.fr>                    +#+  +:+       +#+         #
#                                                 +#+#+#+#+#+   +#+            #
#    Created: 2019/02/28 21:18:25 by dromansk          #+#    #+#              #
#    Updated: 2020/10/23 16:11:00 by alan             ###   ########.fr        #
#                                                                              #
# **************************************************************************** #

NAME = gbmu

SRCS = ./src/*.cc

COMP = g++ -O3

L = -lSDL2

all: $(NAME)

debug: COMP += -DDEBUG_PRINT_ON
debug: re

$(NAME):
	$(COMP) $(SRCS) -o $(NAME) $(O) $(L)

clean:
	rm -rf $(NAME)
	rm -rf roms/*.sav

fclean: clean
	rm -rf $(NAME)

re: fclean all

lre: fclean
	$(COMP) -fsanitize=address $(SRCS) -o $(NAME) $(O) $(L)

vre: fclean
	$(COMP) -g -pg $(SRCS) -o $(NAME) $(O) $(L)

test: re
	./$(NAME) ./roms/Tetris.gb > log.txt
