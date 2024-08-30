CFLAG = -std=c11 -Wall -Werror

SRCS = main.c tcpHandler.c udpHandler.c readHTMLFileToBuffer.c neighbour.c

OBJS = $(SRCS:.c=.o)

all: station-server

station-server: $(OBJS)
	cc $(CFLAG) -o station-server $(OBJS)

%.o: %.c
	cc $(CFLAG) -c $< -o $@


main.o: main.c tcpServer.h udpServer.h neighbour.h
tcpHandler.o: tcpHandler.c tcpServer.h neighbour.h udpServer.h
udpHandler.o: udpHandler.c udpServer.h neighbour.h tcpServer.h
readHTMLFileToBuffer.o: readHTMLFileToBuffer.c tcpServer.h
neighbour.o: neighbour.c neighbour.h

.PHONY: clean
clean:
	rm -f station-server *.o
