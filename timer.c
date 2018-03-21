#include <stdio.h>
#include <time.h>
#include <unistd.h>

#include "httpd.h"

/* @brief Count down timer display
 *
 * @param[in] sec number of seconds to wait
 * @param[in] wait_str display string while waiting
 */
void wait_countdown_timer(const char *wait_str, int sec)
{
	int i;

	i = sec;

	do {
		printf("%s%3d(s) number_of_clients = %lu", wait_str, i,
			num_of_clients());
		fflush(stdout);
		i--;
		sleep(1);
		printf("\r");
	} while (i != 0);
}
