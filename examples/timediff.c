#include <stdio.h>
#include <time.h>   // for time()
#include <unistd.h> // for sleep()

/* compile with 'gcc -lm timediff.c -o timediff' */

// C program to find the execution time of code
int main()
{
	time_t begin = time(NULL);

	// do some stuff here
	sleep(3);

	time_t end = time(NULL);

	// calculate elapsed time by finding difference (end - begin)
	printf("Time elpased is %d seconds", (end - begin));

	return 0;
}
