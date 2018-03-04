#ifndef SQL_API_H
#define SQL_API_H

#include <sqlite3.h>
#include "json_parser.h"

#define SQL_DATABASE "cmarket.db"
#define COIN_REFERENCE "coin_reference"

struct coin_status {
	char *coin_id;
	char *col1;
	int col1_rank;
	char *col2;
	int col2_rank;

	struct coin_status *next;
};

struct coin_status_base {
	int status_count;
	struct coin_status *first;
	struct coin_status *last;
};

extern pthread_mutex_t sql_db_access;

extern sqlite3 *open_main_db(void);
extern void close_main_db(sqlite3 *db);

extern void init_coin_history_table(sqlite3 *db, struct coin_entry_base *coin_base);
extern int fetch_last_updated_column(sqlite3 *db);

extern void shift_columns(sqlite3 *db, const char *col1, const char *col2);
extern void fill_column(sqlite3 *db, struct coin_entry_base *coin_base);

extern struct coin_status_base *fetch_duration(sqlite3 *db, const char *col1,
	const char *col2);
extern void print_coin_status_base(struct coin_status_base *sb);
extern void free_coin_status_base(struct coin_status_base *sb);

#endif
