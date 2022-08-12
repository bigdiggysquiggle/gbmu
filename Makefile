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

SRCS = ./src/cart.cpp ./src/cpu.cpp ./src/mmu.cpp ./src/gb.cpp ./src/debugger.cpp ./src/main.cpp ./src/ppu.cpp

PPU_STUB_SRCS = ./src/cart.cpp ./src/cpu.cpp ./src/mmu.cpp ./src/gb.cpp ./src/debugger.cpp ./src/main.cpp ./src/ppu_stub.cpp

COMP = g++ -O3

#FLAGS = -Wall -Werror -Wextra -Wshadow -pedantic -O2

L = -lSDL2

all: $(NAME)

debug: COMP += -DDEBUG_PRINT_ON #-g -pg
debug: vre

$(NAME):
	$(COMP) $(SRCS) $(FLAGS) -o $(NAME) $(L)

old: fclean
	$(COMP) $(OLD_SRCS) $(FLAGS) -o $(NAME) $(L)

clean:
	rm -rf $(NAME)
	rm -rf roms/*.sav

fclean: clean
	rm -rf $(NAME)

re: fclean all

lre: fclean
	$(COMP) -fsanitize=address $(SRCS) -o $(NAME) $(L)

vre: fclean
	$(COMP) -g -pg $(SRCS) -o $(NAME) $(L)

test: re
	./$(NAME) ./roms/Tetris.gb > log.txt

vtest: vre
	valgrind --track-origins=yes --leak-check=full ./gbmu ~/Downloads/gb/Super Mario Land.gb

diff:
	make debug; ./gbmu ./roms/'Super Mario Land.gb' -o log.txt -f binjgb -d cpu; ./binjgb/out/Debug/binjgb-debugger -t ./roms/'Super Mario Land.gb' > olog.txt ; diff --suppress-common-lines -y log.txt olog.txt > difflog.txt

dlog: debug
	./gbmu ./roms/'Super Mario Land.gb' -o log.txt -f default -d cpu
