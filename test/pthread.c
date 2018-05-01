/**
 * @file pthread.c
 *
 * https://stackoverflow.com/a/49721947/4881848
 */

#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>

pthread_t tid[2];
int counter;
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

void *trythis(void *arg)
{
	pthread_mutex_lock(&lock);

	counter += 1;
	printf("\n Job %d has started\n", counter);

	sleep(5);

	printf("\n Job %d has finished\n", counter);

	pthread_mutex_unlock(&lock);

	return NULL;
}

int main(void)
{
	int i = 0;
	int error;
/*
	if (pthread_mutex_init(&lock, NULL) != 0) {
		printf("\n mutex init has failed\n");
		return 1;
	}*/

	while (i < 2) {
		error = pthread_create(&(tid[i]), NULL, &trythis, NULL);
		if (error != 0)
			printf("\nThread can't be created :[%s]", strerror(error));
		i++;
	}

	pthread_join(tid[0], NULL);
	pthread_join(tid[1], NULL);

	return 0;
}
