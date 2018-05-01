/**
 * @file cmarketcap.h
 */

#ifndef CMARKETCAP_H
#define CMARKETCAP_H

#include <pthread.h>

/**
 * @brief Remember the functioning global data of the program.
 *
 * @detail This global structure is used by multiple threads of
 *		   this program in order to handle unexpected errors
 *		   and fetch global changes of the program.
 */
struct global_data_handle {
	int httpd_sockfd;	/*!< global socked fd for listening incomig connections */
};

extern char *prog_name; /**< Program name */
extern pthread_mutex_t shift_column_locker;
extern struct global_data_handle global_data_handle;

/*!
	This locks others while 'cmarketcap.c' shift columns using
	shift_columns(). This segment solve synchronization
	problems between section 'cmarketcap.c' and 'httpd.c'.
	Think what happens if 'cmarketcap.c' section updates while
	'httpd.c' process client requests.
*/
#define LOCK_SHIFT_COLUMN_LOCKER pthread_mutex_lock(&shift_column_locker)
#define UNLOCK_SHIFT_COLUMN_LOCKER pthread_mutex_unlock(&shift_column_locker)

#endif 
