CURR_WORK_DIR=$(shell pwd)
PREV_DIR=$(shell dirname $(CURR_WORK_DIR))

CFLAGS=-Wall -g -DCM_DEBUG_ -DCM_TESTING_ -I$(PREV_DIR)
LDFLAGS=-lpthread -lsqlite3 -lcurl -ljansson

CC=gcc

TARGETS=signal_handler_test http_parser url_token_test

url_token_test: httpd.o cm_debug.o sql_api.o cmarketcap.o json_parser.o timer.o url_token_test.c
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

httpd.o: $(PREV_DIR)/httpd.c
	$(CC) $(CFLAGS) -c $^ -o ./$@

cm_debug.o: $(PREV_DIR)/cm_debug.c
	$(CC) $(CFLAGS) -c $^ -o ./$@

sql_api.o: $(PREV_DIR)/sql_api.c
	$(CC) $(CFLAGS) -c $^ -o ./$@

cmarketcap.o: $(PREV_DIR)/cmarketcap.c
	$(CC) $(CFLAGS) -c $^ -o ./$@

json_parser.o: $(PREV_DIR)/json_parser.c
	$(CC) $(CFLAGS) -c $^ -o ./$@

timer.o: $(PREV_DIR)/timer.c
	$(CC) $(CFLAGS) -c $^ -o ./$@

http_parser: http_parser.c

signal_handler_test: signal_handler.o signal_handler_test.c
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

signal_handler.o: $(PREV_DIR)/signal_handler.c
	$(CC) $(CFLAGS) -c $^ -o ./$@

clean:
	rm -rf *.o $(TARGETS)

