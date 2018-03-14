#ifndef CMARKETCAP_H
#define CMARKETCAP_H

#include <pthread.h>

struct global_data_handle {
	int httpd_sockfd;
};

extern char *prog_name;
extern pthread_mutex_t shift_column_locker;
extern struct global_data_handle global_data_handle;

#endif 
