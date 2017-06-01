##
## @file makefile
##
## @brief this is the makefile for procdb
##
## @author Ulrike Schaefer 1327450
##
## @date 21.05.2017
##
##

CC = gcc 
CFLAGS=-Wall -std=c99 -pedantic -D_BSD_SOURCE -D_SVID_SOURCE -D_POSIX_C_SOURCE=200809 -g -lrt -lpthread

.PHONY: all clean

all: procdb-server procdb-client

$.o: $.c
	$(CC) $(CFLAGS) $^

clean:
	rm -f procdb-server procdb-server.o procdb-client procdb-client.o

debug: CFLAGS += -DENDEBUG
debug: all



