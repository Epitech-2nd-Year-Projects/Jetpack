##
## EPITECH PROJECT, 2025
## Makefile
## File description:
## Makefile
##

SRC_SERVER = src/Server/main.cpp

SRC_CLIENT = src/Client/main.cpp

OBJ_SRC_SERVER = $(SRC_SERVER:.cpp=.o)
OBJ_SRC_CLIENT = $(SRC_CLIENT:.cpp=.o)
OBJ_SRC = $(OBJ_SRC_SERVER) $(OBJ_SRC_CLIENT)

CFLAGS = -Wall -Wextra -Werror -std=c++20

CXX ?= g++
RM ?= rm -f

NAME_SERVER = jetpack_server
NAME_CLIENT = jetpack_client

.PHONY: all server client clean fclean re

all: server client

server: $(OBJ_SRC_SERVER)
	$(CXX) $(OBJ_SRC_SERVER) $(CFLAGS) -o $(NAME_SERVER)

client: $(OBJ_SRC_CLIENT)
	$(CXX) $(OBJ_SRC_CLIENT) $(CFLAGS) -o $(NAME_CLIENT)

clean:
	$(RM) $(OBJ_SRC)

fclean: clean
	$(RM) $(NAME_SERVER) $(NAME_CLIENT)

re: fclean all
