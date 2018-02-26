CFLAGS=-Wall -DCM_DEBUG_ -g -lpthread -lsqlite3

TARGETS=httpd

all: sql_api.o httpd

httpd: httpd.c ../cm_debug.c ../sql_api.c

sql_api.o: ../sql_api.c ../sql_api.h

clean:
	rm -rf *.o $(TARGETS)
