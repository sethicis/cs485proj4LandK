CC = gcc
CFLAGS = -Wall -g 
LDFLAGS = -lpthread -lcurl -lc
G++ = g++

OBJS = proxy.o csapp.o

all: proxy

proxy: $(OBJS) csapp.h
	$(G++) $(OBJS) -o proxy $(LDFLAGS)

csapp.o: csapp.c
	$(CC) $(CFLAGS) -c csapp.c

proxy.o: proxy.cc
	$(G++) $(CFLAGS) -c proxy.cc
logger.o: logger.cc
	$(G++) $(CFLAGS) -c logger.cc

clean:
	rm -f *~ *.o proxy core

