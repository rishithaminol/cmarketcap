#include <stdio.h>
#include <stdlib.h>
#include <sqlite3.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>

#include "sql_api.h"
#include "cm_debug.h"

/* @brief lock db access */
pthread_mutex_t sql_db_access = PTHREAD_MUTEX_INITIALIZER;
#define LOCK_DB_ACCESS pthread_mutex_lock(&sql_db_access)
#define UNLOCK_DB_ACCESS pthread_mutex_unlock(&sql_db_access)

void free_coin_status_base(struct coin_status_base *sb)
{
	struct coin_status *stat1, *stat2;

	stat1 = sb->first;
	free(sb);

	while (stat1 != NULL) {
		free(stat1->coin_id);
		free(stat1->col1);
		free(stat1->col2);

		stat2 = stat1->next;
		free(stat1);
		stat1 = stat2;
	}
}

void print_coin_status_base(struct coin_status_base *sb)
{
	printf("number of entries = %d\n", sb->status_count);

	struct coin_status *t;

	t = sb->first;

	while (t != NULL) {
		printf("{\n"
			   "	\"coin_id\": \"%s\",\n"
			   "	\"%s_rank\": %d\n"
			   "	\"%s_rank\": %d\n"
			   "},\n",
			   t->coin_id, t->col1, t->col1_rank, t->col2, t->col2_rank);
		t = t->next;
	}
}

struct coin_status *mk_coin_status(const char *coin_id, const char *col1,
	int col1_rank, const char *col2, int col2_rank)
{
	struct coin_status *coin_stat;

	coin_stat = (struct coin_status *)malloc(sizeof(struct coin_status));

	coin_stat->coin_id = (coin_id != NULL) ? strdup(coin_id) : NULL;
	coin_stat->col1 = (col1 != NULL) ? strdup(col1) : NULL;
	coin_stat->col1_rank = col1_rank;
	coin_stat->col2 = (col2 != NULL) ? strdup(col2) : NULL;
	coin_stat->col2_rank = col2_rank;

	coin_stat->next = NULL;

	return coin_stat;
}

struct coin_status_base *init_coin_status_base()
{
	struct coin_status_base *coin_stat_base;

	coin_stat_base = (struct coin_status_base *)malloc(
		sizeof(struct coin_status_base));

	coin_stat_base->status_count = 0;
	coin_stat_base->first = NULL;
	coin_stat_base->last = NULL;

	return coin_stat_base;
}

void append_coin_status(struct coin_status_base *sb, struct coin_status *st)
{
	if (sb->first == NULL) {/* first and last NULL */
		sb->first = st;
		sb->last = sb->first; /* initially first == last */
	} else {
		sb->last->next = st;
		sb->last = sb->last->next;
	}

	sb->status_count++;
}

/**
 * @brief Return two columns of data as a difference of two times. 
 *
 * col1 = min_0, col2 = min_10. behaves like new_coin_entry_base 
 *
 * @return 'struct coin_status_base *''
 */
struct coin_status_base *fetch_duration(sqlite3 *db, const char *col1,
	const char *col2)
{
	char *sql;
	sqlite3_stmt *stmt_1;
	/* @brief return row values */
	int ret_col1;
	int ret_col2;
	char *ret_coin_key;

	struct coin_status_base *coin_stat_base = init_coin_status_base();

	LOCK_DB_ACCESS;

	sql = sqlite3_mprintf("SELECT %s, %s, coin_key "
					"FROM coin_history "
					"WHERE %s < %s AND %s > 0 "
					"ORDER BY %s;", col1, col2, col1, col2, col1, col1);

	if (sqlite3_prepare_v2(db, sql, -1, &stmt_1, 0) != SQLITE_OK)
		CM_ERROR("%s\n", sqlite3_errmsg(db));

	while (1) {
		int rc;
		struct coin_status *s;

		rc = sqlite3_step(stmt_1);
		if(rc == SQLITE_ROW) { /* new row of data ready */
			ret_col1 = sqlite3_column_int(stmt_1, 0);
			ret_col2 = sqlite3_column_int(stmt_1, 1);
			ret_coin_key = (char *)sqlite3_column_text(stmt_1, 2);

			s = mk_coin_status((const char *)ret_coin_key,
				(const char *)col1,
				ret_col1,
				(const char *)col2,
				ret_col2);
			append_coin_status(coin_stat_base, s);

//			DEBUG_MSG("%s:%d:%d\n", ret_coin_key, ret_col1, ret_col2);
		} else if (rc == SQLITE_DONE) { /* sqlite3_step() has finished executing */
			DEBUG_MSG("sqlite3 execution done\n");
			break;
		} else /*if (rc == SQLITE_ERROR)*/ {
			CM_ERROR("%s\n", sqlite3_errmsg(db));
			break;
		}
	}

	UNLOCK_DB_ACCESS;

	sqlite3_reset(stmt_1);
	sqlite3_finalize(stmt_1);
	sqlite3_free(sql);

	if (coin_stat_base->first == NULL)
		CM_ERROR("Everything is NULL\n");

	return coin_stat_base;
}

/* min_0, min_5, min_10, min_15, min_30, hr_1, hr_4, hr_6, hr_12, hr_24 */
void fetch_range_level1(const char *coin_id, sqlite3 *db, int sockfd)
{
	char *sql;
	sqlite3_stmt *stmt_1;
	int rc;
	char tempstr[255];

	LOCK_DB_ACCESS;

	sql = sqlite3_mprintf("SELECT min_0, min_5, min_10, min_15, min_30, hr_1, hr_4, hr_6, hr_12, hr_24 "
						  "FROM coin_history "
						  "WHERE coin_key = '%s';", coin_id);

	if (sqlite3_prepare_v2(db, sql, -1, &stmt_1, 0) != SQLITE_OK)
		CM_ERROR("%s\n", sqlite3_errmsg(db));

	rc = sqlite3_step(stmt_1);
	if(rc == SQLITE_ROW) { /* new row of data ready */
		sprintf(tempstr, "{ "
			   "\t\"min_0\": %d, "
			   "\t\"min_5\": %d, "
			   "\t\"min_10\": %d, "
			   "\t\"min_15\": %d, "
			   "\t\"min_30\": %d, "
			   "\t\"hr_1\": %d, "
			   "\t\"hr_4\": %d, "
			   "\t\"hr_6\": %d, "
			   "\t\"hr_12\": %d, "
			   "\t\"hr_24\": %d "
			   "}",
			   sqlite3_column_int(stmt_1, 0),
			   sqlite3_column_int(stmt_1, 1),
			   sqlite3_column_int(stmt_1, 2),
			   sqlite3_column_int(stmt_1, 3),
			   sqlite3_column_int(stmt_1, 4),
			   sqlite3_column_int(stmt_1, 5),
			   sqlite3_column_int(stmt_1, 6),
			   sqlite3_column_int(stmt_1, 7),
			   sqlite3_column_int(stmt_1, 8),
			   sqlite3_column_int(stmt_1, 9));
		write(sockfd, tempstr, strlen(tempstr));
	}

	UNLOCK_DB_ACCESS;

	sqlite3_finalize(stmt_1);
	sqlite3_free(sql);
}

/* min_0, hr_24, day_2, day_7 */
void fetch_range_level2(const char *coin_id, sqlite3 *db, int sockfd)
{
	char *sql;
	sqlite3_stmt *stmt_1;
	int rc;
	char tempstr[255];

	LOCK_DB_ACCESS;

	sql = sqlite3_mprintf("SELECT min_0, hr_24, day_1, day_2, day_3, day_4, day_5, day_6, day_7 "
						  "FROM coin_history "
						  "WHERE coin_key = '%s';", coin_id);

	if (sqlite3_prepare_v2(db, sql, -1, &stmt_1, 0) != SQLITE_OK)
		CM_ERROR("%s\n", sqlite3_errmsg(db));

	rc = sqlite3_step(stmt_1);
	if(rc == SQLITE_ROW) { /* new row of data ready */
		sprintf(tempstr, "{ "
			   "\t\"min_0\": %d, "
			   "\t\"hr_24\": %d, "
			   "\t\"day_1\": %d, "
			   "\t\"day_2\": %d, "
			   "\t\"day_3\": %d, "
			   "\t\"day_4\": %d, "
			   "\t\"day_5\": %d, "
			   "\t\"day_6\": %d, "
			   "\t\"day_7\": %d "
			   "}",
			   sqlite3_column_int(stmt_1, 0),
			   sqlite3_column_int(stmt_1, 1),
			   sqlite3_column_int(stmt_1, 2),
			   sqlite3_column_int(stmt_1, 3),
			   sqlite3_column_int(stmt_1, 4),
			   sqlite3_column_int(stmt_1, 5),
			   sqlite3_column_int(stmt_1, 6),
			   sqlite3_column_int(stmt_1, 7),
			   sqlite3_column_int(stmt_1, 8)
			   );
		write(sockfd, tempstr, strlen(tempstr));
	}

	UNLOCK_DB_ACCESS;

	sqlite3_finalize(stmt_1);
	sqlite3_free(sql);
}

/**
 * @brief Return two columns of data as a difference of two times. 
 *
 * col1 = min_0, col2 = min_10. behaves like new_coin_entry_base 
 *
 * @return 'struct coin_status_base *''
 */
struct coin_status_base *fetch_entire_rank(sqlite3 *db)
{
	char *sql;
	sqlite3_stmt *stmt_1;
	/* @brief return row values */
	char *ret_coin_key;
	int ret_col_rank;

	struct coin_status_base *coin_stat_base = init_coin_status_base();

	LOCK_DB_ACCESS;

	sql = sqlite3_mprintf("SELECT coin_key, min_0 "
						  "FROM coin_history "
						  "WHERE min_0 > 0 "
						  "ORDER BY min_0;");

	if (sqlite3_prepare_v2(db, sql, -1, &stmt_1, 0) != SQLITE_OK)
		CM_ERROR("%s\n", sqlite3_errmsg(db));

	while (1) {
		int rc;
		struct coin_status *s;

		rc = sqlite3_step(stmt_1);
		if(rc == SQLITE_ROW) { /* new row of data ready */
			ret_coin_key = (char *)sqlite3_column_text(stmt_1, 0);
			ret_col_rank = sqlite3_column_int(stmt_1, 1);

			s = mk_coin_status((const char *)ret_coin_key,
				NULL,
				ret_col_rank,
				NULL,
				0);
			append_coin_status(coin_stat_base, s);

//			DEBUG_MSG("%s:%d:%d\n", ret_coin_key, ret_col1, ret_col2);
		} else if (rc == SQLITE_DONE) { /* sqlite3_step() has finished executing */
			DEBUG_MSG("sqlite3 execution done\n");
			break;
		} else /*if (rc == SQLITE_ERROR)*/ {
			CM_ERROR("%s\n", sqlite3_errmsg(db));
			break;
		}
	}

	UNLOCK_DB_ACCESS;

	sqlite3_reset(stmt_1);
	sqlite3_finalize(stmt_1);
	sqlite3_free(sql);

	if (coin_stat_base->first == NULL)
		CM_ERROR("Everything is NULL\n");

	return coin_stat_base;
}

/** @brief Fill the given column.
 *
 * Updates the 'time_stamp' table. parameter col always
 * takes a name from an array from the caller. eg:- min_0
 * Always this function updates min_0 column only according
 * to the program behaviour.
 *
 * @param col Column name to be filled on 'coin_history'
 * @param coin_base Coin base data structure
 * @param db Openned database
 */
void fill_column(sqlite3 *db, struct coin_entry_base *coin_base)/*,
	const char *col)*/
{
	char *sql, *sql2;
	struct coin_entry *t;
	sqlite3_stmt *stmt_1, *stmt_2;
	const char *col = "min_0";

	t = coin_base->first;

	LOCK_DB_ACCESS;

	/* before filling set all values in min_0 column to '-1' */
	sql = sqlite3_mprintf(
		"UPDATE coin_history SET %s = -1",	col);

	if (sqlite3_prepare_v2(db, sql, -1, &stmt_1, 0) != SQLITE_OK)
		CM_ERROR("%s\n", sqlite3_errmsg(db));

	DEBUG_MSG("setting all values to -1\n");
	if (sqlite3_step(stmt_1) != SQLITE_DONE)
		CM_ERROR("%s\n", sqlite3_errmsg(db));

	sqlite3_free(sql);
	sqlite3_finalize(stmt_1);

	/* @todo performance optimization needed */
	while (t != NULL) {
		sql = sqlite3_mprintf(
			"SELECT min_0 FROM coin_history WHERE coin_key='%s'", t->id);

		if (sqlite3_prepare_v2(db, sql, -1, &stmt_1, 0) != SQLITE_OK) {
			CM_ERROR("%s\n", sqlite3_errmsg(db));
		}

		if (sqlite3_step(stmt_1) != SQLITE_ROW) { /* New coin arrived */
			sql2 = sqlite3_mprintf(
			"INSERT INTO coin_history (coin_key) VALUES ('%s')", t->id);

			if (sqlite3_prepare_v2(db, sql2, -1, &stmt_2, 0) != SQLITE_OK) {
				CM_ERROR("%s\n", sqlite3_errmsg(db));
			}

			if (sqlite3_step(stmt_2) != SQLITE_DONE) {
				CM_ERROR("%s\n", sqlite3_errmsg(db));
			}

			sqlite3_free(sql2);
			sqlite3_finalize(stmt_2);
		}

		sqlite3_free(sql);
		sqlite3_finalize(stmt_1);

		sql = sqlite3_mprintf(
			"UPDATE coin_history SET %s = %d WHERE coin_key = '%s'",
			col, atoi(t->rank), t->id);

		if (sqlite3_prepare_v2(db, sql, -1, &stmt_1, 0) != SQLITE_OK) {
			CM_ERROR("%s\n", sqlite3_errmsg(db));
		}

//		DEBUG_MSG("%s\n", sql);
		if (sqlite3_step(stmt_1) != SQLITE_DONE) {
			CM_ERROR("%s\n", sqlite3_errmsg(db)); /* what happens if
													 new coin arrives */
			break;
		}

		sqlite3_free(sql);
		sqlite3_finalize(stmt_1);

		t = t->next;
	}

	sql = sqlite3_mprintf(
		"UPDATE time_stamps SET time_stamp = %lu WHERE column_name = '%s'",
		 coin_base->derived_time, col);

	sqlite3_prepare_v2(db, sql, -1, &stmt_1, 0); /* update 'time_stamp' */

//	DEBUG_MSG("updating time stamps %s\n", sql);
	if (sqlite3_step(stmt_1) != SQLITE_DONE)
		CM_ERROR("%s\n", sqlite3_errmsg(db));

	UNLOCK_DB_ACCESS;

	sqlite3_free(sql);
	sqlite3_finalize(stmt_1);

	sqlite3_db_cacheflush(db);
}

/* @brief Copy one column to another. Copy 'col2' into 'col1' */
void shift_columns(sqlite3 *db, const char *col1, const char *col2)
{
	char *sql;
	sqlite3_stmt *stmt_1;

	LOCK_DB_ACCESS;

	sql = sqlite3_mprintf("UPDATE coin_history SET `%s` = `%s`", col1, col2);

	DEBUG_MSG("shifting columns -- %s\n", sql);

	if (sqlite3_prepare_v2(db, sql, -1, &stmt_1, 0) != SQLITE_OK) {
		CM_ERROR("%s\n", sqlite3_errmsg(db));
		exit(EXIT_FAILURE);
	}

	if (sqlite3_step(stmt_1) != SQLITE_DONE) {
		CM_ERROR("%s\n", sqlite3_errmsg(db));
		exit(EXIT_FAILURE);
	}

	sqlite3_free(sql);
	sqlite3_finalize(stmt_1);

	/* Shifting timestamps */
	sql = sqlite3_mprintf(
		"UPDATE time_stamps "
		"SET time_stamp = (SELECT time_stamp FROM "
		"time_stamps WHERE column_name = '%s') "
		"WHERE column_name = '%s';",
		col2, col1);

	DEBUG_MSG("shifting time stamps -- %s\n", sql);

	if (sqlite3_prepare_v2(db, sql, -1, &stmt_1, 0) != SQLITE_OK) {
		CM_ERROR("%s\n", sqlite3_errmsg(db));
		exit(EXIT_FAILURE);
	}

	/* This function will never return SQLITE_OK. */
	if (sqlite3_step(stmt_1) != SQLITE_DONE) {
		CM_ERROR("%s\n", sqlite3_errmsg(db));
		exit(EXIT_FAILURE);
	}

	sleep(2);
	sqlite3_db_cacheflush(db);

	UNLOCK_DB_ACCESS;

	sqlite3_free(sql);
	sqlite3_finalize(stmt_1);
}

/** @brief Initialize 'coin_history' table's 'coin_id' field
 *
 * @todo This function should have a way of updating newly arrived coins.
 * 		 And there should be a way of informing dropped coins
 */
void init_coin_history_table(sqlite3 *db, struct coin_entry_base *coin_base)
{
	char *sql;
	sqlite3_stmt *stmt_1;
	struct coin_entry *t;

	LOCK_DB_ACCESS;

	t = coin_base->first;
	while (t != NULL) {
		sql = sqlite3_mprintf(
			"INSERT INTO coin_history (coin_key) VALUES ('%s')", t->id);

		sqlite3_prepare_v2(db, sql, -1, &stmt_1, 0);

		if (sqlite3_step(stmt_1) != SQLITE_DONE)
			CM_ERROR("%s\n", sqlite3_errmsg(db));

		DEBUG_MSG("%s\n", sql);

		sqlite3_free(sql);
		sqlite3_finalize(stmt_1);

		t = t->next;
	}

	UNLOCK_DB_ACCESS;
}

sqlite3 *open_main_db(void)
{
	sqlite3 *db;

	if (sqlite3_open(SQL_DATABASE, &db) != SQLITE_OK) {
		sqlite3_close(db);
		DEBUG_MSG("%s\n", sqlite3_errmsg(db));
		db = NULL;
	}

	return db;
}

void close_main_db(sqlite3 *db)
{
	if (db != NULL)
		sqlite3_close(db);
}
