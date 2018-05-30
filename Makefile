CC = gcc
CFLAGS = -Wall -Werror -Wextra -pedantic -std=c11
#LDFLAGS = -l json

all: router

router: router.c
	#$(CC) $(CFLAGS) router.c -o router $(LDFLAGS)
	$(CC) $(CFLAGS) router.c -o router

clean: router
	rm router
