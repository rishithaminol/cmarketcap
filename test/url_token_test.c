/* @file url_toke_test.c
 *
 * section to test url tokenization features in httpd.c
 * test this section with 'valgrind'
 */

#include <stdio.h>

#include "httpd.h"

int main()
{
	char tok1[] = "/any/path?hello=world&rishitha=minol";
	struct uri_base *x;

	x = tokenize_uri(tok1);
	print_uri_base(x);
	free_uri_base(x);

	return 0;
}
