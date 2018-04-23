CFLAGS=-Wall -g $(shell pkg-config --cflags jansson libcurl) $(shell mysql_config --cflags) -DCM_DEBUG_
LDFLAGS=$(shell pkg-config --libs jansson libcurl) $(shell mysql_config --libs) -lpthread

CC=gcc

TARGETS=cmarketcap
DOXYGEN=doxygen


cmarketcap: mysql_api.o signal_handler.o timer.o httpd.o cm_debug.o json_parser.o cmarketcap.c
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

mysql_api.o: mysql_api.c mysql_api.h

signal_handler.o: signal_handler.c signal_handler.h

timer.o: timer.c timer.h

httpd.o: httpd.c httpd.h

cm_debug.o: cm_debug.c cm_debug.h

json_parser.o: json_parser.c json_parser.h

doxygen:
	$(DOXYGEN) cmarketcap.doxy

clean:
	rm -rf *.o $(TARGETS) doc/doxygen/*

