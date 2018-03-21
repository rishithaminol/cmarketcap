#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <stdlib.h>
#include <time.h> /* time() */

#include "cmarketcap.h"
#include "json_parser.h"
#include "mysql_api.h"
#include "cm_debug.h"
#include "httpd.h"
#include "timer.h"

pthread_mutex_t shift_column_locker = PTHREAD_MUTEX_INITIALIZER;

struct global_data_handle global_data_handle;

/* @brief coin_history column map */
char *col_names[] = {
	"min_0", "min_5", "min_10", "min_15", "min_20", "min_25", "min_30",
	"min_35", "min_40", "min_45", "min_50", "min_55", "hr_1",
	"hr_2", "hr_3", "hr_4", "hr_5", "hr_6", "hr_7", "hr_8", "hr_9",
	"hr_10", "hr_11", "hr_12", "hr_13", "hr_14", "hr_15", "hr_16",
	"hr_17", "hr_18", "hr_19", "hr_20", "hr_21", "hr_22", "hr_23",
	"hr_24", "day_1", "day_2", "day_3", "day_4", "day_5", "day_6", "day_7" //43rd column
};

char *prog_name = NULL; /**< Program name */

/* @brief callback function  */
void *__cb_update_database(void *db_)
{
	struct coin_entry_base *coin_base;

	/* counter variables */
	int min_col = 0; /**! minutes column counter */
	int min_hour_trig = 0; /* minutes and hour trigger */
	int hour_col = 0;
	time_t time_diff;

	MYSQL *db = (MYSQL *)db_;

	pthread_detach(pthread_self());

	while (1) {
		DEBUG_MSG("Starting to count..\n");
		time_diff = time(NULL); /* === clock starts === */

		LOCK_SHIFT_COLUMN_LOCKER;

		/* detects 24 hour completed and shifts hour table */
		if (hour_col == 24) {/* 36th column filled */
			int j;
			for (j = 42; j > 35; j--)
				shift_columns(db, col_names[j], col_names[j - 1]);

			hour_col--;
		}

		if (min_hour_trig == 12) { /* executes after filling 12 columns. */
			int j;
			for (j = min_hour_trig + hour_col; j > 11; j--)
				shift_columns(db, col_names[j], col_names[j - 1]);

			hour_col++; /**! filled an 1 hour column */
		}

		if (min_hour_trig > 0) { /* Executes from the second step of the loop */
			int j;
			for (j = min_col; j > 0; j--)
				shift_columns(db, col_names[j], col_names[j - 1]);

			if (min_hour_trig == 12)
				min_hour_trig = 0;  /* reset 5min columns. restart from first column */
		}

		if (min_col > 10)
			min_col = 10;

		coin_base = new_coin_entry_base();
		cm_update_table(db, coin_base);
		min_col++; /* already filled a column */
		min_hour_trig++;
		DEBUG_MSG("min_col = %d, hour_col = %d, min_hour_trig = %d\n", min_col, hour_col, min_hour_trig);
		free_entry_base(coin_base);

		UNLOCK_SHIFT_COLUMN_LOCKER;

		time_diff = time(NULL) - time_diff; /* === clock ends === */

		DEBUG_MSG("%d seconds spent, waiting %d seconds\n", time_diff, (300 - time_diff));
		if ((300 - time_diff) < 0)
			CM_ERROR("database update took too much time..\n");
		else
			wait_countdown_timer("waiting -- ", 300 - time_diff);
	}

	pthread_exit(NULL);
}

#ifndef CM_TESTING_ /* main() function should be disabled while unit tetsting */

int main(int argc, char *argv[])
{
	MYSQL *db;
	pthread_t update_database_id;

	pthread_mutex_init(&mysql_db_access, NULL);
	pthread_mutex_init(&shift_column_locker, NULL);
	init_httpd_mutexes();

	global_data_handle.httpd_sockfd = 0;

	prog_name = *argv;

	db = open_main_db();
	if (db == NULL) {
		CM_ERROR("Database error\n");
		exit(1);
	}
	/*struct coin_entry_base *x = new_coin_entry_base();
      init_coin_history_table(db, x);
      free_entry_base(x);
      exit(0);
*/
	pthread_create(&update_database_id, NULL, __cb_update_database, (void *)db);
	__cb_main_thread(db);

	close_main_db(db);
	DEBUG_MSG("main database closed\n");

	return 0;
} /* main */

#endif
