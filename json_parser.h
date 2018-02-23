#ifndef JSON_PARSER_H
#define JSON_PARSER_H

#include <time.h>

struct coin_entry {
	int index;

	char *id;
	char *name;
	char *symbol;
	char *rank;
	char *price_usd;

	struct coin_entry *next;
};

struct coin_entry_base {
	int entry_count;
	time_t derived_time;
	struct coin_entry *first;
	struct coin_entry *last;
};

extern void free_entry_base(struct coin_entry_base *eb);
extern void print_entries(struct coin_entry_base *eb);
extern struct coin_entry_base *new_coin_entry_base();

#endif
