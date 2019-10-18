CC := $(TOOLPREFIX)gcc
CFLAGS=-Wall -Wextra -O2


make:
	${CC} ${CFLAGS} ./src/server.c -o server
# 	${CC} ${CFLAGS} client.c -o client

clean: 
	rm -f server client
