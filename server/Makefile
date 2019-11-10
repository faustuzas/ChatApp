CC 	   = gcc
CFLAGS = -g -Wall
SRC    = main.c utils.c
OBJS   = $(SRC:.c=.o)

main: $(OBJS)
	$(CC) $(OBJS) -g -o server
	rm *.o