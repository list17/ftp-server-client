CC = gcc
CFLAGS = -Wall
TARGET = server
SRCS = src/server.c
INC = -I./header

OBJS = $(SRCS:.c=.o)
 
$(TARGET):$(OBJS)
	$(CC) -o $@ $^
 
clean:
	rm -rf $(TARGET) $(OBJS)
 
%.o:%.c
	$(CC) $(CFLAGS) $(INC) -o $@ -c $<
