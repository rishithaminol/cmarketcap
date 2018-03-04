CURR_WORK_DIR=$(shell pwd)
PREV_DIR=$(shell dirname $(CURR_WORK_DIR))

CFLAGS=-Wall -g -I$(PREV_DIR)

CC=gcc

TARGETS=signal_handler_test

signal_handler_test: signal_handler.o signal_handler_test.c
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

signal_handler.o: $(PREV_DIR)/signal_handler.c
	$(CC) $(CFLAGS) -c $^ -o ./$@

clean:
	rm -rf *.o $(TARGETS)

