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
	struct url_base *x;

	x = tokenize_url(tok1);
	print_url_base(x);
	free_url_base(x);

	return 0;
}
