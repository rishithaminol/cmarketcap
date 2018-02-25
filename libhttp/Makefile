CFLAGS=-Wall -DCM_DEBUG_ -g -lpthread

TARGETS=httpd

all: httpd

httpd: httpd.c ../cm_debug.c

clean:
	rm -rf *.o $(TARGETS)
