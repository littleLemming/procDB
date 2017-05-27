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
DEFS = -D_BSD_SOURCE -D_SVID_SOURCE -D_POSIX_C_SOURCE=200809
CFLAGS = -Wall -g -lrt -lpthread -std=c99 -pedantic $(DEFS)

.PHONY: all clean

all: server client

server: server.o procdb.h

client: client.o procdb.h

$.o: $.c 
	$( CC ) $( CFLAGS ) -c -o $@ $<

clean:
	rm -f server server.o client client.o

debug: CFLAGS += -DENDEBUG
debug: all



