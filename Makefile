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

SRCS = ./src/cart.cc ./src/cpu.cc ./src/mmu.cc ./src/gb.cc ./src/debugger.cc ./src/main.cc ./src/ppu.cc

OLD_SRCS = ./src/cart.cc ./src/cpu_old.cc ./src/mmu.cc ./src/gb.cc ./src/debugger.cc ./src/main.cc ./src/ppu.cc

PPU_STUB_SRCS = ./src/cart.cc ./src/cpu.cc ./src/mmu.cc ./src/gb.cc ./src/debugger.cc ./src/main.cc ./src/ppu_stub.cc

COMP = clang++ -O3

#FLAGS = -Wall -Werror -Wextra -Wshadow -pedantic -O2

L = -lSDL2

all: $(NAME)

debug: COMP += -DDEBUG_PRINT_ON
debug: vre

$(NAME):
	$(COMP) $(SRCS) $(FLAGS) -o $(NAME) $(O) $(L)

old: fclean
	$(COMP) $(OLD_SRCS) $(FLAGS) -o $(NAME) $(O) $(L)

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
