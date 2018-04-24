#ifndef CMARKETCAP_H
#define CMARKETCAP_H

#include <pthread.h>

/**
 * @brief Remember the file descriptor of openned socket.
 *
 * This global structure is used for close the connection
 * in unexpected exits.
 */
struct global_data_handle {
	int httpd_sockfd;
};

extern char *prog_name;
extern pthread_mutex_t shift_column_locker;
extern struct global_data_handle global_data_handle;

/*!
	This locks others while 'cmarketcap.c' shift columns using
	shift_columns(). This segment solve synchronization
	problems between section 'cmarketcap.c' and 'httpd.c'
*/
#define LOCK_SHIFT_COLUMN_LOCKER pthread_mutex_lock(&shift_column_locker)
#define UNLOCK_SHIFT_COLUMN_LOCKER pthread_mutex_unlock(&shift_column_locker)

#endif 
