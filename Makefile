CC = gcc
CFLAGS = -Wall -g 
LDFLAGS = -lpthread
G++ = g++

OBJS = proxy.o csapp.o

all: proxy

proxy: $(OBJS) csapp.h
	$(G++) $(OBJS) -o proxy $(LDFLAGS)

csapp.o: csapp.c
	$(CC) $(CFLAGS) -c csapp.c

proxy.o: proxy.cc
	$(G++) $(CFLAGS) -c proxy.cc

clean:
	rm -f *~ *.o proxy core

