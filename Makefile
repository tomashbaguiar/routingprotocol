CC=gcc
FLAGS=-Wall -Werror -Wextra -pedantic -std=c11

all: router

router: router.c
	$(CC) $(FLAGS) router.c -o router

clean: router
	rm router
