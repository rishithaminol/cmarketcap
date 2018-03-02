#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <stdlib.h>
#include <time.h> /* time() */

#include "cmarketcap.h"
#include "json_parser.h"
#include "sql_api.h"
#include "cm_debug.h"
#include "httpd.h"
#include "timer.h"

/* @brief coin_history column map */
char *col_names[] = {
	"min_0", "min_5", "min_10", "min_15", "min_20", "min_25", "min_30",
	"min_35", "min_40", "min_45", "min_50", "min_55", "hr_1",
	"hr_2", "hr_3", "hr_4", "hr_5", "hr_6", "hr_7", "hr_8", "hr_9",
	"hr_10", "hr_11", "hr_12", "hr_13", "hr_14", "hr_15", "hr_16",
	"hr_17", "hr_18", "hr_19", "hr_20", "hr_21", "hr_22", "hr_23",
	"hr_24"
};

char *prog_name = NULL; /**< Program name */

/* @brief callback function  */
void *__cb_update_database(void *db_)
{
	struct coin_entry_base *coin_base;
	int col_rounds = 0; /**! holds the number of columns currently updated */
	int col_one_hour = 0;
	time_t time_diff;

	sqlite3 *db = (sqlite3 *)db_;

	pthread_detach(pthread_self());

	do {
		DEBUG_MSG("Starting to count..\n");
		time_diff = time(NULL); /* === clock starts === */

		if (col_rounds == 12) { /* executes after filling 12 columns */
			int j;
			for (j = col_rounds + col_one_hour; j > 11; j--)
				shift_columns(db, col_names[j], col_names[j - 1]);

			col_rounds = 0; /* reset 5min columns. restart from first column */
			col_one_hour++; /**! filled an 1 hour column */

			if (col_one_hour == 24) /* 36th column filled */
				col_one_hour--;
		}

		if (col_rounds > 0) { /* Executes from the second step of the loop */
			int j;
			for (j = col_rounds; j > 0; j--) {
				/* at the time of shifting, time_stamp data should be
				 * shifted
				 */
				shift_columns(db, col_names[j], col_names[j - 1]);
			}
		}

		coin_base = new_coin_entry_base();
		fill_column(db, coin_base);
		col_rounds++; /* already filled a column */
		DEBUG_MSG("col_rounds = %d\n", col_rounds);
		free_entry_base(coin_base);

		time_diff = time(NULL) - time_diff; /* === clock ends === */
		DEBUG_MSG("%d seconds spent, waiting %d seconds\n", time_diff, (300 - time_diff));
		if ((300 - time_diff) < 0)
			CM_ERROR("database update took too much time..\n");
		else
			wait_countdown_timer("waiting -- ", 300 - time_diff);
	} while (1);

	pthread_exit(NULL);
}

int main(int argc, char *argv[])
{
	sqlite3 *db;
	pthread_t update_database_id;

	pthread_mutex_init(&sql_db_access, NULL);

	prog_name = *argv;

	db = open_main_db();
	if (db == NULL) {
		CM_ERROR("Database error\n");
		exit(1);
	}

	pthread_create(&update_database_id, NULL, __cb_update_database, (void *)db);
	__cb_main_thread(db);

	close_main_db(db);
	DEBUG_MSG("main database closed\n");

	return 0;
} /* main */