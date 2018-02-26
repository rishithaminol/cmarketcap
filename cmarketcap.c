#include <stdio.h>
#include <unistd.h>
#include <pthread.h>

#include "cmarketcap.h"
#include "json_parser.h"
#include "sql_api.h"
#include "cm_debug.h"

/* @brief coin_history column map */
char *col_names[] = {"min_0", "min_5", "min_10", "min_15", "min_20", "min_25", "min_30",
		"min_35", "min_40", "min_45", "min_50", "min_55", "hr_1",
		"hr_2", "hr_3", "hr_4", "hr_5", "hr_6", "hr_7", "hr_8", "hr_9",
		"hr_10", "hr_11", "hr_12", "hr_13", "hr_14", "hr_15", "hr_16",
		"hr_17", "hr_18", "hr_19", "hr_20", "hr_21", "hr_22", "hr_23",
		"hr_24"};

char *prog_name = NULL; /**< Program name */

/* @brief callback function  */
void *update_database(void *db_)
{
	struct coin_entry_base *coin_base;
	int col_5min = 0; /**! holds the number of columns currently updated */

	sqlite3 *db = (sqlite3 *)db_;

	pthread_detach(pthread_self());

	int i = 0;
	do {
		if (col_5min > 0) { /* Executes from the second step of the loop */
			int j;
			for (j = col_5min; j > 0; j--) {
				/* at the time of shifting, time_stamp data should be
				 * shifted
				 */
				shift_columns(db, col_names[j], col_names[j - 1]); 
				/* shift_time_stamps(db, ) */
				DEBUG_MSG("%s shifted to %s\n", col_names[j - 1], col_names[j]);
			}
		}

		coin_base = new_coin_entry_base();
		fill_column(db, coin_base);
		col_5min = ++i; /* already filled a column */
		DEBUG_MSG("col_5min = %d\ni = %d\n", col_5min, i);

		free_entry_base(coin_base);
		sleep(300);
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

	pthread_create(&update_database_id, NULL, update_database, (void *)db);

	update_database(db);

	close_main_db(db);

	pthread_exit(NULL);
} /* main */
