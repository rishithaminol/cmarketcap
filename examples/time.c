#include <time.h>
#include <stdio.h>

int main()
{
	time_t t;

	t = time(NULL);

	printf("%lu\n", t);

    return 0;
}
