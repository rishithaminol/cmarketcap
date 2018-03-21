#ifndef SQL_API_H
#define SQL_API_H

#include <my_global.h>
#include <mysql.h>
#include "json_parser.h"

#define SQL_DATABASE "cmarketcap_db"

struct coin_status {
	char *coin_id;
	char *coin_symbol;
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

extern pthread_mutex_t mysql_db_access;

extern MYSQL *open_main_db(void);
extern void close_main_db(MYSQL *db);
extern void init_coin_history_table(MYSQL *db, struct coin_entry_base *coin_base);

extern void free_coin_status_base(struct coin_status_base *sb);
extern void print_coin_status_base(struct coin_status_base *sb);
extern void cm_update_table(MYSQL *db, struct coin_entry_base *coin_base);
extern void shift_columns(MYSQL *db, const char *col1, const char *col2);
extern struct coin_status_base *fetch_entire_rank(MYSQL *db);
extern struct coin_status_base *fetch_duration(MYSQL *db, const char *col1,
	const char *col2);
extern void fetch_range_level1(const char *coin_id, MYSQL *db, int sockfd);
extern void fetch_range_level2(const char *coin_id, MYSQL *db, int sockfd);

#endif
