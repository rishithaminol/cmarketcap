CFLAGS=-Wall -g $(shell pkg-config --cflags jansson sqlite3 libcurl) -DCM_DEBUG_
LDFLAGS=$(shell pkg-config --libs jansson sqlite3 libcurl) -lpthread

CC=gcc

TARGETS=cmarketcap

cmarketcap: signal_handler.o timer.o httpd.o cm_debug.o sql_api.o json_parser.o cmarketcap.c
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

signal_handler.o: signal_handler.c signal_handler.h

timer.o: timer.c timer.h

httpd.o: httpd.c httpd.h

cm_debug.o: cm_debug.c cm_debug.h

sql_api.o: sql_api.c sql_api.h

json_parser.o: json_parser.c json_parser.h

clean:
	rm -rf *.o $(TARGETS)