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

all: procdb-server procdb-client

procdb-server: procdb-server.o procdb.h

procdb-client: procdb-client.o procdb.h

$.o: $.c 
	$( CC ) $( CFLAGS ) -c -o $@ $<

clean:
	rm -f procdb-server procdb-server.o procdb-client procdb-client.o

debug: CFLAGS += -DENDEBUG
debug: all



