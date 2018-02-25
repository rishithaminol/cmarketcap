#include <string.h>
#include <stdio.h>

int main()
{
	char str[80] = "This is - www.tutorialspoint.com - website";
	char *token;
	char *rest = str;

	/* get the first token */
	token = strtok_r(rest, "-", &rest);
	printf("%s\n", token);
	printf("rest = %s\n", rest);
	token = strtok_r(rest, "-", &rest);
	printf("%s\n", token);
	printf("rest = %s\n", rest);
	printf("%s\n", str);
	return (0);
}
