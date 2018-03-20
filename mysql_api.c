#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <my_global.h>
#include <mysql.h>

#include "mysql_api.h"
#include "json_parser.h"
#include "cm_debug.h"

/* @brief lock db access */
pthread_mutex_t mysql_db_access = PTHREAD_MUTEX_INITIALIZER;
#define LOCK_DB_ACCESS   pthread_mutex_lock(&mysql_db_access)
#define UNLOCK_DB_ACCESS pthread_mutex_unlock(&mysql_db_access)

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

	coin_stat->coin_id   = (coin_id != NULL) ? strdup(coin_id) : NULL;
	coin_stat->col1      = (col1 != NULL) ? strdup(col1) : NULL;
	coin_stat->col1_rank = col1_rank;
	coin_stat->col2      = (col2 != NULL) ? strdup(col2) : NULL;
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
	coin_stat_base->first        = NULL;
	coin_stat_base->last         = NULL;

	return coin_stat_base;
}

void append_coin_status(struct coin_status_base *sb, struct coin_status *st)
{
	if (sb->first == NULL) {/* first and last NULL */
		sb->first = st;
		sb->last  = sb->first; /* initially first == last */
	} else {
		sb->last->next = st;
		sb->last       = sb->last->next;
	}

	sb->status_count++;
}

MYSQL *open_main_db(void)
{
	MYSQL *db = mysql_init(NULL);

	if (db == NULL) {
		CM_ERROR("%s\n", mysql_error(db));
		exit(1);
	}

	if (mysql_real_connect(db, "localhost", "root", "rootroot01",
	  SQL_DATABASE, 0, NULL, 0) == NULL)
	{
		CM_ERROR("%s\n", mysql_error(db));
		mysql_close(db);
		exit(1);
	}

	return db;
}

void close_main_db(MYSQL *db)
{
	if (db != NULL)
		mysql_close(db);
}

/** @brief Initialize 'coin_history' table's 'coin_id' field
 *
 * @todo This function should have a way of updating newly arrived coins.
 *       And there should be a way of informing dropped coins
 */
void init_coin_history_table(MYSQL *db, struct coin_entry_base *coin_base)
{
	char sql[255];
	struct coin_entry *t;

	LOCK_DB_ACCESS;

	t = coin_base->first;
	while (t != NULL) {
		sprintf(sql, "INSERT INTO `coin_history` (coin_key) VALUES ('%s')", t->id);

		if (mysql_query(db, sql) != 0) {
			CM_ERROR("%s\n", mysql_error(db));
			UNLOCK_DB_ACCESS;
			return;
		}

		t = t->next;
	}

	UNLOCK_DB_ACCESS;
}

/** @brief Fill first column of the table.
 *
 * Updates the 'time_stamp' table. parameter col always
 * takes a name from an array from the caller. eg:- min_0
 * Always this function updates min_0 column only according
 * to the program behaviour. shift columns before updating.
 *
 * @param col Column name to be filled on 'coin_history'
 * @param coin_base Coin base data structure
 * @param db Openned database
 */
void cm_update_table(MYSQL *db, struct coin_entry_base *coin_base)
{
	char sql[255], sql2[255];
	struct coin_entry *t;
	const char *col = "min_0";

	t = coin_base->first;

	LOCK_DB_ACCESS;

	/* before filling set all values in min_0 column to '-1' */
	sprintf(sql, "UPDATE coin_history SET %s = 0", col);

	DEBUG_MSG("setting all values to '0'\n");
	if (mysql_query(db, sql) != 0) {
		CM_ERROR("%s\n", mysql_error(db));
		UNLOCK_DB_ACCESS;
		return;
	}
	mysql_free_result(mysql_store_result(db));

	/* @todo performance optimization needed */
	while (t != NULL) {
		sprintf(sql, "SELECT `min_0` FROM `coin_history` WHERE coin_key='%s';", t->id);

		if (mysql_query(db, sql) != 0) { /* New coin arrived */
			sprintf(sql2,
			  "INSERT INTO `coin_history` (coin_key) VALUES ('%s');", t->id);

			if (mysql_query(db, sql2) != 0)
				CM_ERROR("%s\n", mysql_error(db));
		}
		mysql_free_result(mysql_store_result(db));

		sprintf(sql,
		  "UPDATE `coin_history` SET %s = %d WHERE coin_key = '%s';",
		  col, atoi(t->rank), t->id);

		if (mysql_query(db, sql) != 0) {
			CM_ERROR("%s\n", mysql_error(db));
			break;
		}

		t = t->next;
	}

	sprintf(sql,
	  "UPDATE `time_stamps` SET time_stamp = %lu WHERE column_name = '%s'",
	  coin_base->derived_time, col);

	if (mysql_query(db, sql) != 0)
		CM_ERROR("%s\n", mysql_error(db));

	UNLOCK_DB_ACCESS;
} /* cm_update_table */

/* @brief Copy one column to another. Copy 'col2' into 'col1' */
void shift_columns(MYSQL *db, const char *col1, const char *col2)
{
	char sql[255];

	LOCK_DB_ACCESS;

	sprintf(sql, "UPDATE coin_history SET `%s` = `%s`", col1, col2);

	DEBUG_MSG("shifting columns -- %s\n", sql);
	if (mysql_query(db, sql) != 0)
		CM_ERROR("%s\n", mysql_error(db));

	/* Shifting timestamps */
	sprintf(sql,
	  "UPDATE `time_stamps` "
	  "SET time_stamp = (SELECT time_stamp FROM "
	  "`time_stamps` WHERE column_name = '%s') "
	  "WHERE column_name = '%s';",
	  col2, col1);

	DEBUG_MSG("shifting time stamps -- %s\n", sql);
	if (mysql_query(db, sql) != 0)
		CM_ERROR("%s\n", mysql_error(db));

	UNLOCK_DB_ACCESS;
}

/**
 * @brief Return two columns of data as a difference of two times.
 *
 * col1 = min_0, col2 = min_10. behaves like new_coin_entry_base
 *
 * @return 'struct coin_status_base *'' NULL on error
 */
struct coin_status_base *fetch_entire_rank(MYSQL *db)
{
	char sql[255];
	/* @brief return row values */
	char *ret_coin_key;
	int ret_col_rank;

	MYSQL_RES *result;
	MYSQL_ROW row;

	struct coin_status_base *coin_stat_base = init_coin_status_base();

	LOCK_DB_ACCESS;

	sprintf(sql, "SELECT coin_key, min_0 "
	  "FROM coin_history "
	  "WHERE min_0 > 0 "
	  "ORDER BY min_0;");

	if (mysql_query(db, sql) != 0) {
		CM_ERROR("%s\n", mysql_error(db));
		UNLOCK_DB_ACCESS;
		return NULL;
	}

	result = mysql_store_result(db);
	if (result == NULL) {
		CM_ERROR("%s\n", mysql_error(db));
		UNLOCK_DB_ACCESS;
		return NULL;
	}

	if (mysql_num_fields(result) != 2) { /* this should return 2 columns */
		CM_ERROR("%s\n", mysql_error(db));
		free_coin_status_base(coin_stat_base);
		mysql_free_result(result);
		UNLOCK_DB_ACCESS;
		return NULL;
	}

	while ((row = mysql_fetch_row(result))) {
		struct coin_status *s;

		ret_coin_key = row[0];       /* char */
		ret_col_rank = atoi(row[1]); /* int */

		s = mk_coin_status((const char *)ret_coin_key,
		    NULL,
		    ret_col_rank,
		    NULL,
		    0);
		append_coin_status(coin_stat_base, s);
	}

	UNLOCK_DB_ACCESS;

	mysql_free_result(result);

	if (coin_stat_base->first == NULL)
		CM_ERROR("Everything is NULL\n");

	return coin_stat_base;
} /* fetch_entire_rank */

/**
 * @brief Return two columns of data as a difference of two times.
 *
 * col1 = min_0, col2 = min_10. behaves like new_coin_entry_base
 *
 * @return 'struct coin_status_base *''. NULL on error
 */
struct coin_status_base *fetch_duration(MYSQL *db, const char *col1,
  const char *col2)
{
	char sql[255];
	/* @brief return row values */
	int ret_col1;
	int ret_col2;
	char *ret_coin_key;

	MYSQL_RES *result;
	MYSQL_ROW row;

	struct coin_status_base *coin_stat_base = init_coin_status_base();

	LOCK_DB_ACCESS;

	sprintf(sql, "SELECT %s, %s, coin_key "
	  "FROM coin_history "
	  "WHERE %s < %s AND %s > 0 "
	  "ORDER BY %s;", col1, col2, col1, col2, col1, col1);

	if (mysql_query(db, sql) != 0) {
		CM_ERROR("%s\n", mysql_error(db));
		UNLOCK_DB_ACCESS;
		return NULL;
	}

	result = mysql_store_result(db);
	if (result == NULL) {
		CM_ERROR("%s\n", mysql_error(db));
		UNLOCK_DB_ACCESS;
		return NULL;
	}

	if (mysql_num_fields(result) != 3) { /* this should return 3 columns */
		CM_ERROR("%s\n", mysql_error(db));
		free_coin_status_base(coin_stat_base);
		mysql_free_result(result);
		UNLOCK_DB_ACCESS;
		return NULL;
	}

	while ((row = mysql_fetch_row(result))) {
		struct coin_status *s;

		ret_col1     = atoi(row[0]);
		ret_col2     = atoi(row[1]);
		ret_coin_key = row[2];

		s = mk_coin_status((const char *)ret_coin_key,
		    (const char *)col1,
		    ret_col1,
		    (const char *)col2,
		    ret_col2);
		append_coin_status(coin_stat_base, s);
	}

	UNLOCK_DB_ACCESS;

	mysql_free_result(result);

	if (coin_stat_base->first == NULL)
		CM_ERROR("Everything is NULL\n");

	return coin_stat_base;
} /* fetch_duration */

/* min_0, min_5, min_10, min_15, min_30, hr_1, hr_4, hr_6, hr_12, hr_24 */
void fetch_range_level1(const char *coin_id, MYSQL *db, int sockfd)
{
	char sql[255];
	char tempstr[255];
	MYSQL_RES *result;
	MYSQL_ROW row;

	LOCK_DB_ACCESS;

	sprintf(sql, "SELECT min_0, min_5, min_10, min_15, min_30, hr_1, hr_4, hr_6, hr_12, hr_24 "
	  "FROM coin_history "
	  "WHERE coin_key = '%s';", coin_id);

	if (mysql_query(db, sql) != 0) {
		CM_ERROR("%s\n", mysql_error(db));
		UNLOCK_DB_ACCESS;
		return;
	}

	result = mysql_store_result(db);
	if (result == NULL) {
		CM_ERROR("%s\n", mysql_error(db));
		UNLOCK_DB_ACCESS;
		return;
	}

	row = mysql_fetch_row(result);

	sprintf(tempstr, "{ "
	  "\t\"min_0\": %s, "
	  "\t\"min_5\": %s, "
	  "\t\"min_10\": %s, "
	  "\t\"min_15\": %s, "
	  "\t\"min_30\": %s, "
	  "\t\"hr_1\": %s, "
	  "\t\"hr_4\": %s, "
	  "\t\"hr_6\": %s, "
	  "\t\"hr_12\": %s, "
	  "\t\"hr_24\": %s "
	  "}",
	  row[0],
	  row[1],
	  row[2],
	  row[3],
	  row[4],
	  row[5],
	  row[6],
	  row[7],
	  row[8],
	  row[9]);
	write(sockfd, tempstr, strlen(tempstr));

	UNLOCK_DB_ACCESS;

	mysql_free_result(result);
} /* fetch_range_level1 */

/* min_0, min_5, min_10, min_15, min_30, hr_1, hr_4, hr_6, hr_12, hr_24 */
void fetch_range_level2(const char *coin_id, MYSQL *db, int sockfd)
{
	char sql[255];
	char tempstr[255];
	MYSQL_RES *result;
	MYSQL_ROW row;

	LOCK_DB_ACCESS;

	sprintf(sql, "SELECT min_0, hr_24, day_1, day_2, day_3, day_4, day_5, day_6, day_7 "
	  "FROM coin_history "
	  "WHERE coin_key = '%s';", coin_id);

	if (mysql_query(db, sql) != 0) {
		CM_ERROR("%s\n", mysql_error(db));
		UNLOCK_DB_ACCESS;
		return;
	}

	result = mysql_store_result(db);
	if (result == NULL) {
		CM_ERROR("%s\n", mysql_error(db));
		UNLOCK_DB_ACCESS;
		return;
	}

	row = mysql_fetch_row(result);

	sprintf(tempstr, "{ "
	  "\t\"min_0\": %s, "
	  "\t\"hr_24\": %s, "
	  "\t\"day_1\": %s, "
	  "\t\"day_2\": %s, "
	  "\t\"day_3\": %s, "
	  "\t\"day_4\": %s, "
	  "\t\"day_5\": %s, "
	  "\t\"day_6\": %s, "
	  "\t\"day_7\": %s "
	  "}",
	  row[0],
	  row[1],
	  row[2],
	  row[3],
	  row[4],
	  row[5],
	  row[6],
	  row[7],
	  row[8]);
	write(sockfd, tempstr, strlen(tempstr));

	UNLOCK_DB_ACCESS;

	mysql_free_result(result);
} /* fetch_range_level2 */
