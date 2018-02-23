CFLAGS=-Wall -g -I/home/minol/.local/include -DCM_DEBUG_
LDFLAGS=-L/home/minol/.local/lib -ljansson -lcurl -lsqlite3 -lpthread

TARGETS=cmarketcap

cmarketcap: cm_debug.o sql_api.o json_parser.o cmarketcap.c

cm_debug.o: cm_debug.c cm_debug.h

sql_api.o: sql_api.c sql_api.h

json_parser.o: json_parser.c json_parser.h

clean:
	rm -rf *.o $(TARGETS)
