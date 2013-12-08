#!/bin/sh

echo -e "starting...\n"
gcc -Wall -g -c echo.c echoserveri.c echoclient.c csapp.c
gcc -Wall -g -o echoServer echoserveri.o echo.o csapp.o -lpthread
gcc -Wall -g -o echoClient echoclient.o csapp.o -lpthread
echo -e "complete...\n"
